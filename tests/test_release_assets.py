from __future__ import annotations

import hashlib
import json
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RELEASE = ROOT / "release-assets" / "2.00.00"
RELEASE_203 = ROOT / "release-assets" / "2.03.00"
RELEASE_20302 = ROOT / "release-assets" / "2.03.02"
BUILD_REPORT = ROOT / "reports" / "BUILD_REPORT_2.00.00.json"

EXPECTED = {
    "Alden_AIO_Shop_All_Items_1_Copper_2.00.00.zip": ("v3", 379),
    "All_Mounts_LvL_5_All_Stats_2.00.00.zip": ("legacy", 4990),
    "All_Mounts_LvL_5_Speed_2.00.00.zip": ("legacy", 1248),
    "Steelheart_Horseshoes_20_Stamina_Regen_2.00.00.zip": ("v3", 1),
    "Healthbar_always_on_2.00.00.zip": ("healthbar", 3),
    "Healthbar_always_on_classic_vanilla_single_target_2.00.00.zip": ("healthbar", 3),
    "Healthbar_always_on_vanilla_multitarget_2.00.00.zip": ("healthbar", 3),
}


def test_release_set_is_exact() -> None:
    assert {path.name for path in RELEASE.glob("*.zip")} == set(EXPECTED)


def test_release_hashes_match_published_metadata() -> None:
    checksums = {}
    for line in (RELEASE / "SHA256SUMS.txt").read_text().splitlines():
        digest, filename = line.split(maxsplit=1)
        checksums[filename] = digest

    report = json.loads(BUILD_REPORT.read_text())
    assert set(checksums) == set(EXPECTED)
    assert set(report["packages"]) == set(EXPECTED)

    for filename in EXPECTED:
        actual = hashlib.sha256((RELEASE / filename).read_bytes()).hexdigest()
        assert checksums[filename] == actual
        assert report["packages"][filename]["sha256"] == actual


def test_final_names_and_manifests() -> None:
    for filename, (kind, count) in EXPECTED.items():
        path = RELEASE / filename
        stem = path.stem
        with zipfile.ZipFile(path) as archive:
            assert archive.testzip() is None
            assert all(name.startswith(stem + "/") for name in archive.namelist())
            manifests = [name for name in archive.namelist() if name.endswith(".json")]
            assert len(manifests) == 1
            manifest = json.loads(archive.read(manifests[0]).decode("utf-8-sig"))
            if kind in {"v3"}:
                assert manifest["modinfo"]["title"] == stem
                assert manifest["modinfo"]["version"] == "2.00.00"
                intents = sum(len(target.get("intents", [])) for target in manifest["targets"])
                assert intents == count
            else:
                assert manifest["name"] == stem
                assert manifest["version"] == "2.00.00"
                assert len(manifest["patches"]) == count
            upper = stem.upper()
            assert "_DMM" not in upper
            assert "_RC" not in upper
            assert "_TEST" not in upper
            assert "_R2" not in upper
            assert "_R3" not in upper
            assert "_R4" not in upper
            assert "_R5" not in upper


def test_healthbar_contains_working_overlay() -> None:
    for filename in [name for name in EXPECTED if name.startswith("Healthbar_")]:
        path = RELEASE / filename
        stem = path.stem
        with zipfile.ZipFile(path) as archive:
            names = set(archive.namelist())
            prefix = stem + "/files/"
            assert prefix + "0008/gamedata/binary__/client/bin/skill.pabgb" in names
            assert prefix + "0008/gamedata/binary__/client/bin/skill.pabgh" in names
            assert prefix + "0012/ui/xml/gamemain/play/subtitletagview.html" in names
            assert prefix + "0012/ui/xml/gamemain/play/subtitletagview.css" in names


def test_recipes_match_confirmed_baselines() -> None:
    mounts = [line.strip() for line in (ROOT / "recipes/2.00.00/mount_targets.txt").read_text().splitlines() if line.strip()]
    assert len(mounts) == 380
    assert len(set(mounts)) == 380

    catalog = []
    for line in (ROOT / "recipes/2.00.00/alden_catalog.tsv").read_text().splitlines():
        if line.strip() and not line.startswith("#"):
            catalog.append(tuple(map(int, line.split("\t")[:3])))
    assert len(catalog) == 379
    assert [row[0] for row in catalog] == list(range(379))
    assert len({row[1] for row in catalog}) == 379


def test_mining_helmet_always_on_release() -> None:
    filename = "Mining_Helmet_Always_On_2.03.00.zip"
    package = RELEASE_203 / filename
    checksums = {}
    for line in (RELEASE_203 / "SHA256SUMS.txt").read_text().splitlines():
        digest, checksum_filename = line.split(maxsplit=1)
        checksums[checksum_filename] = digest

    assert hashlib.sha256(package.read_bytes()).hexdigest() == checksums[filename]
    with zipfile.ZipFile(package) as archive:
        assert archive.testzip() is None
        assert set(archive.namelist()) == {
            "MinHook-LICENSE.txt",
            "Mining_Helmet_Always_On_2.03.00.asi",
            "README.txt",
        }
        assert archive.getinfo("Mining_Helmet_Always_On_2.03.00.asi").file_size > 0


def test_mount_20302_releases_are_sealed_and_complete() -> None:
    expected = {
        "All_Mounts_LvL_5_All_Stats_2.03.02.zip": "All_Mounts_LvL_5_All_Stats_2.03.02.asi",
        "All_Mounts_LvL_5_Speed_2.03.02.zip": "All_Mounts_LvL_5_Speed_2.03.02.asi",
    }
    checksums = {}
    for line in (RELEASE_20302 / "SHA256SUMS.txt").read_text().splitlines():
        digest, filename = line.split(maxsplit=1)
        checksums[filename] = digest

    assert set(path.name for path in RELEASE_20302.glob("*.zip")) == set(expected)
    for filename, asi_name in expected.items():
        package = RELEASE_20302 / filename
        assert hashlib.sha256(package.read_bytes()).hexdigest() == checksums[filename]
        with zipfile.ZipFile(package) as archive:
            assert archive.testzip() is None
            assert [entry.filename for entry in archive.infolist()] == [asi_name]
            assert archive.getinfo(asi_name).file_size > 0

    for asi_name in expected.values():
        mod = next(ROOT.glob(f"mods/**/{asi_name}"))
        assert hashlib.sha256(mod.read_bytes()).hexdigest() == checksums[asi_name]


def test_mount_20302_sources_use_the_current_guarded_hook() -> None:
    for source in [
        ROOT / "mods/all-mounts-level-5-all-stats/src/AllMountsLvL5AllStats_20302.c",
        ROOT / "mods/all-mounts-level-5-speed/src/AllMountsLvL5Speed_20302.c",
    ]:
        text = source.read_text()
        assert "imageBase + 0x00E68B5C" in text
        assert "imageBase + 0x00E68F09" in text
        assert "0x4D,0x8B,0x96,0x30,0x02,0x00,0x00,0x49" in text

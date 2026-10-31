"""Bundle the validated Healthbar data and native helper for one DMM ZIP import."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import zipfile

VERSION = "2.04.02"
BUILD = "26329202"
ASI = "Healthbar Multitarget.asi"
ASI_SHA = "6a8cd4012899d734016ece0f452349b21556e0bfea04c1302e41d36e380b5455"
VARIANTS = {
    "vanilla_multitarget": ("Vanilla Multitarget", "Classic red bars with a dark frame, multiple enemies.", True),
    "classic_vanilla_single_target": ("Classic Vanilla Single Target", "Classic red bars with a dark frame, one target.", False),
    "Caites": ("Caites Multitarget", "Compact thin red bars without the thick frame, multiple enemies.", True),
}
UI_SHA = {
    "vanilla_multitarget": ("0c5d73a5bf41ae660159d401a900ba0c425037544ee0c7589b97f719edeec9c1", "df27c0bb1abd0323497cf41974a28865191533864e6510044a615a5b88997ea4"),
    "classic_vanilla_single_target": ("dac9fa9905b29b55418371b2287b29598fbeef17a518159961b442ac23bad80c", "99a3daea5f3a7a260efa84e9d009457e024ce6dab2ee451cc9c76b351346579b"),
    "Caites": ("0c5d73a5bf41ae660159d401a900ba0c425037544ee0c7589b97f719edeec9c1", "82ed06b4f93cbacf26ce94be76c09940777131def32d45ac512b6ab01ddfc985"),
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def instructions(label: str, description: str, native: bool) -> str:
    helper = (
        f"3. In ASI Mods, ensure {ASI} is enabled. It is included in this ZIP.\n"
        "   Keep DMM's existing ASI loader; use DMM's loader installation if none is installed.\n"
        if native else "3. This variant does not require a separate Healthbar ASI helper.\n"
    )
    german_helper = (
        "3. Unter ASI Mods den beiliegenden Healthbar-Helfer aktivieren, falls er noch aus ist.\n"
        "   Den vorhandenen ASI-Loader behalten; falls keiner installiert ist, ueber DMM installieren.\n"
        if native else "3. Fuer diese Variante ist kein zusaetzlicher Healthbar-ASI-Helfer erforderlich.\n"
    )
    return (
        f"HEALTHBAR ALWAYS ON - {label}\nRelease 3 | Crimson Desert {VERSION} | Steam build {BUILD}\n\n"
        f"{description}\n\n"
        "INSTALL WITH DMM 4.5.1\n"
        "1. Close the game. Unmount and remove the previous Healthbar variant from DMM.\n"
        "2. Import THIS complete ZIP into DMM and enable the Healthbar data mod.\n"
        + helper +
        "4. Mount and start the game. No manual file copy is required.\n\n"
        "Choose ONE of the three variants. Do not install or enable them together.\n"
        "Switch variants with the game closed; mount again and restart.\n"
        "The same bundled helper handles both Multitarget styles. It stays inactive with Classic.\n"
        "No separate helper download, custom installer or additional configuration is needed.\n"
        "If upgrading from a manually installed test helper, remove that old manual copy first.\n"
        "Do not keep NativeFix1 or duplicate Healthbar helpers in bin64 or scripts.\n"
        "Keep unrelated mods and DmmRuntime.asi installed.\n\n"
        "COMPATIBILITY\n"
        "Built specifically for the Steam build listed above. Other builds are not supported.\n"
        "The helper refuses unknown game executables or altered/missing Healthbar UI files.\n"
        "Other mods editing subtitletagview.html/css or the same skill may conflict.\n"
        "Alden and Steelheart were checked alongside this data.\n"
        "Uninstall: close the game, disable/remove Healthbar data and its helper in DMM, then remount.\n\n"
        "DEUTSCHE KURZANLEITUNG\n"
        "1. Spiel schliessen. Unmount; die bisherige Healthbar-Variante aus DMM entfernen.\n"
        "2. Dieses komplette ZIP in DMM importieren und den Healthbar-Datenmod aktivieren.\n"
        + german_helper +
        "4. Mounten und das Spiel starten. Kein manuelles Kopieren erforderlich.\n"
        "Nur EINE der drei Varianten installieren. Beim Wechsel das Spiel neu starten.\n\n"
        "Credits: Blablup; Caites for the original Persistent HP Bars work and compact presentation.\n"
        "https://www.nexusmods.com/crimsondesert/mods/2406\n"
        "https://www.nexusmods.com/crimsondesert/mods/2409\n"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-dir", type=Path, required=True)
    parser.add_argument("--native", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    native = args.native.read_bytes()
    if sha(native) != ASI_SHA:
        raise ValueError("Not the gameplay-validated NativeFix2 binary")
    args.output.mkdir(parents=True, exist_ok=True)
    records = []
    for variant, (label, description, needs_native) in VARIANTS.items():
        slug = f"Healthbar_always_on_{variant}_{VERSION}"
        source = args.data_dir / f"{slug}.zip"
        with zipfile.ZipFile(source) as z:
            assert z.testzip() is None
            assert all(n.startswith(slug + "/") for n in z.namelist())
            contents = {n[len(slug) + 1:]: z.read(n) for n in z.namelist() if not n.endswith("/")}
        # DMM discovers native plugins at the mod root. Do not nest the ASI in
        # bin64 or put another ZIP inside this importable ZIP.
        assert all(not n.startswith("/") and ".." not in Path(n).parts for n in contents)
        for extension, expected in zip(("html", "css"), UI_SHA[variant], strict=True):
            data = contents[f"files/0012/ui/xml/gamemain/play/subtitletagview.{extension}"]
            assert sha(data) == expected, "Unvalidated UI payload"
        manifest_name = slug + ".json"
        manifest = json.loads(contents[manifest_name])
        patches_before = manifest["patches"]
        manifest["name"] = f"Healthbar - {label}"
        manifest["description"] = f"{description} Crimson Desert {VERSION}, Steam {BUILD}. " + (
            "Includes the required Multitarget ASI helper; enable both components in DMM."
            if needs_native else "DMM data mod; no separate Healthbar ASI helper required."
        )
        contents[manifest_name] = (json.dumps(manifest, indent=2) + "\n").encode()
        contents["README.txt"] = instructions(label, description, needs_native).encode()
        original_report = json.loads(contents["BUILD_VALIDATION.txt"])
        original_report["note"] = "Build-time validation only; gameplay results are recorded separately in the release notes."
        contents["BUILD_VALIDATION.txt"] = (json.dumps(original_report, indent=2) + "\n").encode()
        if needs_native:
            contents[ASI] = native
            contents["NATIVE_LICENSE.txt"] = (Path(__file__).resolve().parents[2] /
                "mods/healthbar-always-on/source/native/LICENSE.txt").read_bytes()
        assert sum(n.endswith(".json") for n in contents) == 1
        destination = args.output / f"{slug}.zip"
        with zipfile.ZipFile(destination, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
            for name, data in sorted(contents.items()):
                info = zipfile.ZipInfo(name, (2026, 10, 31, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                z.writestr(info, data, compresslevel=9)
        with zipfile.ZipFile(destination) as z, zipfile.ZipFile(source) as original:
            assert z.testzip() is None
            assert set(z.namelist()) == set(contents)
            for name in z.namelist():
                if name.startswith("files/"):
                    assert z.read(name) == original.read(slug + "/" + name)
            assert json.loads(z.read(manifest_name))["patches"] == patches_before
            assert ([n for n in z.namelist() if n.endswith(".asi")] == [ASI]) == needs_native
            assert not any(n.endswith((".dll", ".exe", ".zip", ".log")) for n in z.namelist())
        records.append(dict(variant=variant, label=label, file=destination.name,
                            bytes=destination.stat().st_size, sha256=sha(destination.read_bytes()),
                            original_data_zip_sha256=sha(source.read_bytes()),
                            native_sha256=ASI_SHA if needs_native else None,
                            game_payloads_identical=True, character_patches_identical=True,
                            only_one_json_manifest=True, nested_archives=False))
    (args.output / "SHA256SUMS.txt").write_text(
        "".join(f"{r['sha256']}  {r['file']}\n" for r in records), encoding="utf-8")
    (args.output / "PACKAGING_VALIDATION.json").write_text(
        json.dumps(dict(game_version=VERSION, steam_build=BUILD, nexus_version="3",
                        dmm_combined_import_tested=False, files=records), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(records, indent=2))


if __name__ == "__main__":
    main()

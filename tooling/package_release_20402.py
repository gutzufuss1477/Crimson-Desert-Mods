"""Package the approved 2.04.02 native/data payloads; standard library only."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
TAG = "2.04.02_b26329202"
EXE_SHA = "fec07be1b725a46624d31fcd74bf3730ea809765226cea6bde01607ab2d79f66"
DATA_PAYLOAD_HASHES = {'Alden_AIO_Shop_All_Items_1_Copper_2.04.02': {'Alden_AIO_Shop_All_Items_1_Copper_2.04.02/Alden_AIO_Shop_All_Items_1_Copper_2.04.02.json': '86bdacbfe74950a356349dff3a476b2942f4f34206fcc83f3bbbb2864b3baac7', 'Alden_AIO_Shop_All_Items_1_Copper_2.04.02/files/0008/gamedata/binarystaticinfo__/bin/storeinfo.staticinfobody': 'ecd2ed74fdfc7607a771943f568b21525333ce1e679d32f7cdb15044006a63c5', 'Alden_AIO_Shop_All_Items_1_Copper_2.04.02/files/0008/gamedata/binarystaticinfo__/bin/storeinfo.staticinfoheader': 'bc40d745bc069b7ed2466947c74ab5e7be30f218236c58fa0e4e9bf586e02223'}, 'Steelheart_Horseshoes_20_Stamina_Regen_2.04.02': {'Steelheart_Horseshoes_20_Stamina_Regen_2.04.02/Steelheart_Horseshoes_20_Stamina_Regen_2.04.02.json': 'cbe402d284e83c9ab419c87511ddb556bed49460fde8d9ba4a96d254cebb4759'}}

MODS = [
    ("Mining_Helmet_Always_On", "1.3.2", "mods/mining-helmet-always-on/source",
     "src/runtime/out", "18e81c08a961b3d911b8b34c343d175b1b3a0cb4a40d2671df31615626e7c9c1",
     ["Mining_Helmet_Always_On.ini", "MinHook-LICENSE.txt"],
     "Highlights ore with any headgear. Range 10-120 (default 60); GlowStrength 0.5-1.0 (default 0.5).\n"
     "INI changes reload while playing. Remove the old separate Mining_Ore_Group_Range test ASI.\n"
     "Status: %LOCALAPPDATA%\\MiningHelmetAlwaysOn\\MH132\\latest-status.txt\n"),
    ("Bank_Refresh", "1.2.2", "mods/faster-bank-refresh-gold-and-bonds/source",
     "src/out", "09797e2c95fb8b5a56ba789760aafb53cb88cbb1c57b6650710dda0bbd1d4290",
     ["Bank_Refresh.ini", "LICENSE.txt", "src/vendor/minhook/LICENSE.txt"],
     "Gold and bonds are configurable independently; default 15 IN-GAME minutes each.\n"
     "Bond starts/reinvestment remain manual. Payouts and investment risk are unchanged.\n"
     "The native bank tick may briefly delay refresh after the displayed countdown reaches zero.\n"),
    ("AllMountsLvL5AllStats", "12", "mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/variants/all-mounts-level-5-all-stats",
     "@tooling/out", "f35464c3a1fcfe4fc5a6779319ef4c8d171355e4c2defd44bcc32174fdc79a1d", [],
     "Level 5 mount stats. Use only one mount-stat variant: AllStats OR Speed, never both.\n"
     "Steelheart Horseshoes is a separate data mod and is not required for this ASI.\n"),
    ("AllMountsLvL5Speed", "12", "mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/variants/all-mounts-level-5-speed",
     "@tooling/out", "36b45e1422219c91fec27ced80f7122e9d004e5b8190e448073f40f4086cd63e", [],
     "Level 5 mount movement speed only; acceleration, turning and jump are unchanged.\n"
     "Use only one mount-stat variant: Speed OR AllStats, never both.\n"
     "Steelheart Horseshoes is a separate data mod and is not required for this ASI.\n"),
]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    output = parser.parse_args().output
    output.mkdir(parents=True, exist_ok=True)
    sums = []
    for name, version, folder, binary_dir, expected, extras, detail in MODS:
        source = ROOT / folder
        binary_root = ROOT / binary_dir[1:] if binary_dir.startswith("@") else source / binary_dir
        filename = f"{name}_{TAG}.asi"
        payload = (binary_root / filename).read_bytes()
        if digest(payload) != expected:
            raise ValueError(f"{filename}: not the approved release binary")
        files = {filename: payload}
        for extra in extras:
            label = "MinHook-LICENSE.txt" if "vendor/minhook" in extra else Path(extra).name
            files[label] = (source / extra).read_bytes()
        gameplay_tested = name != "AllMountsLvL5Speed"
        validation = ("Author confirmed all three mods working on 2026-10-31." if gameplay_tested else
                      "Speed-only passed static/host/loader checks. It shares the gameplay-tested AllStats hook; a separate Speed-only gameplay test is pending.")
        report = dict(mod=name, mod_version=version, game_version="2.04.02", steam_build="26329202",
                      exe_sha256=EXE_SHA, in_game_tested=gameplay_tested,
                      gameplay_validation=validation,
                      files_sha256={k: digest(v) for k, v in files.items()})
        files["BUILD_VALIDATION.txt"] = (json.dumps(report, indent=2) + "\n").encode()
        validation_text = ("Updated native hooks and build checks for this exact Steam build. Tested in-game.\n\n" if gameplay_tested else
                           "Updated for this exact Steam build. Static/host/loader checks passed.\n"
                           "Uses the same hook as the gameplay-tested AllStats build, restricted to speed.\n"
                           "A separate in-game test of Speed-only is pending.\n\n")
        files["README.txt"] = (f"{name} {version} - Crimson Desert 2.04.02 / Steam build 26329202\n\n"
            + validation_text +
            "INSTALL\n"
            "1. Close the game. Keep your existing x64 ASI loader. If needed, obtain it from:\n"
            "   https://github.com/ThirteenAG/Ultimate-ASI-Loader\n"
            "2. Back up/remove older ASIs for this same mod, including test versions.\n"
            "3. Copy the supplied ASI into Crimson Desert\\bin64 beside CrimsonDesert.exe.\n"
            "4. Copy the supplied INI only if you do not already have one; retain existing settings.\n"
            "5. Start the game normally. DMM and data-archive mounting are not required.\n\n"
            + detail + "\nUNINSTALL\nClose the game and remove this mod's ASI.\n\n"
            "This release targets Steam build 26329202, not every build labelled 2.04.02.\n"
            "Source, build instructions and release hashes:\n"
            "https://github.com/gutzufuss1477/Crimson-Desert-Mods\n").encode()
        target = output / f"{name}_{TAG}.zip"
        with zipfile.ZipFile(target, "w") as z:
            for label, data in sorted(files.items()):
                info = zipfile.ZipInfo(label, (2026, 10, 31, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                z.writestr(info, data, compresslevel=9)
        with zipfile.ZipFile(target) as z:
            assert z.testzip() is None
            assert {n: z.read(n) for n in z.namelist()} == files
        sums.append(f"{digest(target.read_bytes())}  {target.name}\n")
        print(f"{target.name}: {target.stat().st_size} bytes; approved ASI unchanged")
    data_mods = [
        ("Alden_AIO_Shop_All_Items_1_Copper_2.04.02", "16",
         "318688f4c474a77843db9407c2025564b021247623c5994a8e4f8d60aeb3ae3b",
         "Original 379-item catalog, stock and order retained. Catalog purchase prices are 1 Copper.\n"),
        ("Steelheart_Horseshoes_20_Stamina_Regen_2.04.02", "9",
         "bfde1bf52ef9b9454cc3ff850a97707691d3a6d110ae083e114bf04252871553",
         "Steelheart Horseshoes stamina-regeneration equip-buff level increased from 4 to 20.\n"
         "Equip the Steelheart Horseshoes. Can be used with either mount-stat ASI, or alone.\n"),
    ]
    for name, version, expected, detail in data_mods:
        candidate = ROOT / "dist/2.04.02-b26329202" / f"{name}.zip"
        with zipfile.ZipFile(candidate) as archive:
            if archive.testzip():
                raise ValueError(f"Invalid ZIP: {candidate}")
            files = {n: archive.read(n) for n in archive.namelist()}
        payload_hashes = {n: digest(data) for n, data in files.items() if not n.endswith(("/README.txt", "/BUILD_VALIDATION.txt"))}
        if payload_hashes != DATA_PAYLOAD_HASHES[name]:
            raise ValueError(f"{name}: data payload differs from the gameplay-confirmed candidate")
        prefix = name + "/"
        report = json.loads(files[prefix + "BUILD_VALIDATION.txt"])
        report.update(mod_version=version, manager_mount_tested=True, runtime_tested=True,
                      in_game_tested=True, gameplay_validation="Author confirmed working on 2026-10-31.",
                      validated_candidate_sha256=expected)
        files[prefix + "BUILD_VALIDATION.txt"] = (json.dumps(report, indent=2) + "\n").encode()
        files[prefix + "README.txt"] = (
            f"{name} - mod version {version}\nCrimson Desert 2.04.02 / Steam build 26329202\n\n"
            + detail + "\nDMM 4.5.1 mount and in-game operation confirmed by the author.\n\n"
            "INSTALL / UPDATE\n1. Close the game and unmount in DMM.\n"
            "2. Import this ZIP in DMM 4.5.1 or a compatible newer release.\n"
            "3. Disable/remove the previous version of this same mod. Enable the new one.\n"
            "4. Mount, then launch the game. Keep DMM's installed runtime and ASI loader.\n"
            "Do not copy the files/ directory manually into the game.\n\n"
            "UNINSTALL\nClose the game, unmount in DMM, disable/remove this mod, then remount your remaining mods.\n"
            "Uninstalling does not remove items already bought or restore money spent.\n\n"
            "Source, recipes and instructions: https://github.com/gutzufuss1477/Crimson-Desert-Mods\n"
        ).encode()
        target = output / f"{name}.zip"
        with zipfile.ZipFile(target, "w") as archive:
            for label, payload in sorted(files.items()):
                info = zipfile.ZipInfo(label, (2026, 10, 31, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                archive.writestr(info, payload, compresslevel=9)
        with zipfile.ZipFile(target) as archive:
            assert archive.testzip() is None
            assert {n: archive.read(n) for n in archive.namelist()} == files
        sums.append(f"{digest(target.read_bytes())}  {target.name}\n")
        print(f"{target.name}: {target.stat().st_size} bytes; validated data payload unchanged")
    (output / "SHA256SUMS.txt").write_text("".join(sorted(sums)), encoding="utf-8")


if __name__ == "__main__":
    main()

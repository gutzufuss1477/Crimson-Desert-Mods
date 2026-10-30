"""Package the approved ASI payloads; Python standard library only."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
TAG = "2.04.00_b26317560"
EXE_SHA = "d4855c08998638f07fa8fae08bd6249261f10c8fec2215effc29cb339e7fa525"
MODS = [
    ("Mining_Helmet_Always_On", "1.3.1", "mods/mining-helmet-always-on/source",
     "src/runtime/out", "a63f0532fb7354aaeba05be19cdb4047bf353b0fe273dcad81ab6a3e42f6ccbb",
     ["Mining_Helmet_Always_On.ini", "MinHook-LICENSE.txt"],
     "Highlights ore with any headgear. Range 10-120 (default 60); GlowStrength 0.5-1.0 (default 0.5).\n"
     "INI changes reload while playing. Remove the old separate Mining_Ore_Group_Range test ASI.\n"
     "Status: %LOCALAPPDATA%\\MiningHelmetAlwaysOn\\MH131\\latest-status.txt\n"),
    ("Bank_Refresh", "1.2.1", "mods/faster-bank-refresh-gold-and-bonds/source",
     "src/out", "a9ce0a6cb33af21811fc389b551aed838fb2a6250f971d250b122ef33a3c8561",
     ["Bank_Refresh.ini", "LICENSE.txt", "src/vendor/minhook/LICENSE.txt"],
     "Gold and bonds are configurable independently; default 15 IN-GAME minutes each.\n"
     "Bond starts/reinvestment remain manual. Payouts and investment risk are unchanged.\n"
     "The native bank tick may briefly delay refresh after the displayed countdown reaches zero.\n"),
    ("AllMountsLvL5AllStats", "11", "mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/variants/all-mounts-level-5-all-stats",
     "@tooling/out", "84853a8f7d4526ff759d436b5dc0184b14305b9061c5dd37f323deb2e3ae9056", [],
     "Level 5 mount stats. Use only one mount-stat variant: AllStats OR Speed, never both.\n"
     "Steelheart Horseshoes is a separate data mod and is not required for this ASI.\n"),
    ("AllMountsLvL5Speed", "11", "mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/variants/all-mounts-level-5-speed",
     "@tooling/out", "1a47f19999a48cdd7c77f03422da1075b454e6eab0cbff5073a9d7f9607a2962", [],
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
        validation = ("Author confirmed all three mods working on 2026-10-30." if gameplay_tested else
                      "Speed-only passed static/host/loader checks. It shares the gameplay-tested AllStats hook; a separate Speed-only gameplay test is pending.")
        report = dict(mod=name, mod_version=version, game_version="2.04.00", steam_build="26317560",
                      exe_sha256=EXE_SHA, in_game_tested=gameplay_tested,
                      gameplay_validation=validation,
                      files_sha256={k: digest(v) for k, v in files.items()})
        files["BUILD_VALIDATION.txt"] = (json.dumps(report, indent=2) + "\n").encode()
        validation_text = ("Updated native hooks and build checks for this exact Steam build. Tested in-game.\n\n" if gameplay_tested else
                           "Updated for this exact Steam build. Static/host/loader checks passed.\n"
                           "Uses the same hook as the gameplay-tested AllStats build, restricted to speed.\n"
                           "A separate in-game test of Speed-only is pending.\n\n")
        files["README.txt"] = (f"{name} {version} - Crimson Desert 2.04.00 / Steam build 26317560\n\n"
            + validation_text +
            "INSTALL\n"
            "1. Close the game. Keep your existing x64 ASI loader. If needed, obtain it from:\n"
            "   https://github.com/ThirteenAG/Ultimate-ASI-Loader\n"
            "2. Back up/remove older ASIs for this same mod, including test versions.\n"
            "3. Copy the supplied ASI into Crimson Desert\\bin64 beside CrimsonDesert.exe.\n"
            "4. Copy the supplied INI only if you do not already have one; retain existing settings.\n"
            "5. Start the game normally. DMM and data-archive mounting are not required.\n\n"
            + detail + "\nUNINSTALL\nClose the game and remove this mod's ASI.\n\n"
            "This release targets Steam build 26317560, not every build labelled 2.04.00.\n"
            "Source, build instructions and release hashes:\n"
            "https://github.com/gutzufuss1477/Crimson-Desert-Mods\n").encode()
        target = output / f"{name}_{TAG}.zip"
        with zipfile.ZipFile(target, "w") as z:
            for label, data in sorted(files.items()):
                info = zipfile.ZipInfo(label, (2026, 10, 30, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                z.writestr(info, data, compresslevel=9)
        with zipfile.ZipFile(target) as z:
            assert z.testzip() is None
            assert {n: z.read(n) for n in z.namelist()} == files
        sums.append(f"{digest(target.read_bytes())}  {target.name}\n")
        print(f"{target.name}: {target.stat().st_size} bytes; approved ASI unchanged")
    (output / "SHA256SUMS.txt").write_text("".join(sorted(sums)), encoding="utf-8")


if __name__ == "__main__":
    main()

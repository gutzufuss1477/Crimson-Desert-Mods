"""Package already-built native 2.04.00 plugins with configs, licenses and hashes."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

from .build_204 import ROOT, write_json
from .verify_204 import Image, EXE_SHA
from .native_204 import NATIVE_BUILD as STEAM_BUILD, NATIVE_TAG, NATIVE_VERSION as VERSION


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    output = parser.parse_args().output
    output.mkdir(parents=True, exist_ok=True)
    bank = ROOT / "mods/faster-bank-refresh-gold-and-bonds/source"
    mining = ROOT / "mods/mining-helmet-always-on/source"
    mount = ROOT / "mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/variants"
    releases = [
        ("Bank_Refresh", "1.2.2", bank / f"src/out/Bank_Refresh_{NATIVE_TAG}.asi",
         [bank / "Bank_Refresh.ini", bank / "LICENSE.txt", bank / "src/vendor/minhook/LICENSE.txt"],
         [bank / "src/bank_refresh.cpp", bank / "src/bank_core.hpp"],
         "Gold and bonds have independent INI settings. Retain your existing Bank_Refresh.ini.\n"),
        ("Mining_Helmet_Always_On", "1.3.2", mining / f"src/runtime/out/Mining_Helmet_Always_On_{NATIVE_TAG}.asi",
         [mining / "Mining_Helmet_Always_On.ini", mining / "MinHook-LICENSE.txt"],
         [mining / "src/runtime/mining_helmet.cpp", mining / "src/runtime/group_range.hpp", mining / "src/material/material_only.hpp"],
         "Retain your INI. The ore-group range adjustment is integrated: remove the separate old Mining_Ore_Group_Range test ASI.\n"),
    ]
    for variant, name in (("all-mounts-level-5-speed", "AllMountsLvL5Speed"),
                          ("all-mounts-level-5-all-stats", "AllMountsLvL5AllStats")):
        releases.append((name, NATIVE_TAG, ROOT / f"tooling/out/{name}_{NATIVE_TAG}.asi", [],
                         [mount / variant / f"src/{name}_20402.c"],
                         "Choose Speed OR AllStats. Never load both variants together. Steelheart is a separate optional data mod.\n"))
    for name, version, binary, extras, source, detail in releases:
        image = Image(binary)
        # DMM's ASI scanner expects the plugin directly in its mod folder.
        # A bin64 subfolder is suitable for manual copying, but wasn't detected
        # when the original package was imported into DMM 4.4.2.
        files = {binary.name: image.data}
        for path in extras:
            label = "MinHook-LICENSE.txt" if "minhook" in path.parts else path.name
            files[label] = path.read_bytes()
        report = dict(mod=name, mod_version=version, game_version=VERSION, steam_build=STEAM_BUILD,
                      exe_sha256=EXE_SHA, in_game_tested=False,
                      files_sha256={k: hashlib.sha256(v).hexdigest() for k, v in files.items()},
                      source_sha256={p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in source})
        files["BUILD_VALIDATION.txt"] = (json.dumps(report, indent=2)+"\n").encode()
        files["README.txt"] = (f"{name} {version} for Crimson Desert {VERSION}, Steam {STEAM_BUILD}\n\n"
            "Local build candidate. Static/host checks passed; in-game validation is pending.\n"
            "Import via DMM's ASI tab, or copy the ASI and optional INI into game bin64 manually.\n"
            "Close the game and back up existing ASIs/configs before replacing an old plugin.\n"
            "Native ASIs do not require DMM. Leave DmmRuntime.asi installed when using DMM data mods.\n"
            "Do not keep old/test versions active alongside this plugin. An ASI loader is required.\n"
            "Existing INI settings take precedence: copy the supplied INI only if none exists.\n"+detail).encode()
        target = output / f"{name}_{NATIVE_TAG}.zip"
        with zipfile.ZipFile(target, "w", compression=zipfile.ZIP_DEFLATED) as z:
            for filename, data in sorted(files.items()):
                info = zipfile.ZipInfo(filename, (2026, 10, 31, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                z.writestr(info, data, compresslevel=9)
        with zipfile.ZipFile(target) as z:
            if z.testzip() or any(z.read(n) != data for n, data in files.items()):
                raise ValueError(f"ZIP readback failed: {target}")
        write_json(output / f"{name}_{NATIVE_TAG}.validation.json", report | {"package_sha256": hashlib.sha256(target.read_bytes()).hexdigest()})
        print(target)
    archives = sorted(output.glob("*.zip"))
    if len(archives) != 4:
        raise ValueError(f"Use a separate output directory for the four native packages; found {len(archives)} ZIPs")
    (output / "SHA256SUMS.txt").write_text("".join(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n" for p in archives))


if __name__ == "__main__":
    main()

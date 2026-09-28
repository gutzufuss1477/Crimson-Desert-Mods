"""Package the built 1.1.0 ASI without altering other mods or historical releases."""
from __future__ import annotations

import hashlib
import json
import shutil
import struct
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MOD = ROOT / "mods/mining-helmet-always-on"
RELEASE = ROOT / "release-assets/2.03.02"
ASI = "Mining_Helmet_Always_On_2.03.02.asi"
ZIP = "Mining_Helmet_Always_On_2.03.02.zip"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    built = MOD / "src/material/out" / ASI
    data = built.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[:2] != b"MZ" or data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("Expected a PE plugin")
    if struct.unpack_from("<H", data, pe + 4)[0] != 0x8664:
        raise ValueError("Expected an x64 plugin")
    if b"1.1.0\0" not in data or "MH110".encode("utf-16le") not in data:
        raise ValueError("Expected the release-labelled material build")
    if b"test34-automatic-material" in data:
        raise ValueError("Refusing a diagnostic build")
    RELEASE.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(built, MOD / ASI)
    members = {ASI: MOD / ASI, "README.txt": MOD / "README.txt", "CHANGELOG.txt": MOD / "CHANGELOG.txt"}
    # Stable member metadata makes packaging repeatable for the same build.
    with zipfile.ZipFile(RELEASE / ZIP, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, path in sorted(members.items()):
            info = zipfile.ZipInfo(name, (2026, 9, 28, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            content = path.read_text(encoding="utf-8").encode("utf-8") if name.endswith(".txt") else path.read_bytes()
            archive.writestr(info, content)
    with zipfile.ZipFile(RELEASE / ZIP) as archive:
        if archive.testzip() is not None or archive.read(ASI) != data:
            raise ValueError("Archive integrity failure")
    sums_path = RELEASE / "SHA256SUMS.txt"
    sums = {}
    for line in sums_path.read_text(encoding="utf-8").splitlines():
        if line.strip():
            value, name = line.split(maxsplit=1)
            sums[name] = value
    sums[ASI] = digest(MOD / ASI)
    sums[ZIP] = digest(RELEASE / ZIP)
    sums_path.write_text("".join(f"{value}  {name}\n" for name, value in sorted(sums.items())), encoding="utf-8")
    sources = [MOD / "src/build.cmd", *sorted((MOD / "src/material").glob("*.cpp")), *sorted((MOD / "src/material").glob("*.hpp"))]
    report = {
        "mod_version": "1.1.0", "game_version": "2.03.02", "steam_build": "25474236",
        "game_exe_sha256": "57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7",
        "asi": {"file": ASI, "sha256": sums[ASI], "size": len(data)},
        "package": {"file": ZIP, "sha256": sums[ZIP], "members": sorted(members)},
        "source_hash_format": "UTF-8 with LF line endings",
        "sources": {str(path.relative_to(ROOT)).replace("\\", "/"): hashlib.sha256(path.read_text(encoding="utf-8").encode("utf-8")).hexdigest() for path in sources},
        "validation_notes": "VALIDATION_MINING_HELMET_1.1.0.md",
    }
    (ROOT / "reports/BUILD_REPORT_MINING_HELMET_1.1.0.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"package": str(RELEASE / ZIP), "asi_sha256": sums[ASI], "zip_sha256": sums[ZIP]}, indent=2))


if __name__ == "__main__":
    main()

"""Read back data ZIPs and verify their changes against the selected vanilla build."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

from .build_204 import Inputs, PROFILES, BINARY, UI, write_json
from .staticinfo import Table
from .verify_204 import data_packages


def verify(inputs, output, expected_count=5):
    # Validates every byte-patch preimage and composes simultaneous patches,
    # rejecting conflicting writes and changes to record identities/order.
    result = data_packages(inputs, output, expected_count)
    replacement_checks = []
    for package in sorted(output.glob("*.zip")):
        with zipfile.ZipFile(package) as archive:
            prefix = package.stem + "/"
            if prefix + package.stem + ".json" not in archive.namelist():
                # Native ASI layout is checked by data_packages; it has no DMM manifest.
                continue
            report = json.loads(archive.read(prefix + "BUILD_VALIDATION.txt"))
            manifest = json.loads(archive.read(prefix + package.stem + ".json"))
            if (report["game_version"], report["steam_build"], manifest["version"]) != (
                    inputs.version, inputs.steam_build, inputs.version):
                raise ValueError("Package build identity differs")
            for path, digest in report["input_sha256"].items():
                if hashlib.sha256(inputs.read(Path(path))).hexdigest() != digest:
                    raise ValueError("Package source hash differs")
            payloads = {name[len(prefix + "files/"):]: archive.read(name)
                        for name in archive.namelist() if name.startswith(prefix + "files/")}
            healthbar = package.name.startswith("Healthbar_")
            alden = package.name.startswith("Alden_")
            if not healthbar and not alden:
                if not package.name.startswith("Steelheart_") or payloads or len(manifest["patches"]) != 1:
                    raise ValueError("Unexpected Steelheart contents")
                patch = manifest["patches"][0]
                if bytes.fromhex(patch["patched"]) != struct.pack("<II", 1000046, 20):
                    raise ValueError("Wrong Steelheart buff")
                continue
            name, key_size, target = ("skill", 4, 1201) if healthbar else ("storeinfo", 2, 3101)
            body = (BINARY / (name + ".staticinfobody")).as_posix()
            header = (BINARY / (name + ".staticinfoheader")).as_posix()
            expected = {body, header}
            if healthbar:
                expected |= {(UI / ("subtitletagview." + ext)).as_posix() for ext in ("html", "css")}
            if payloads.keys() != expected:
                raise ValueError("Unexpected replacement file set")
            before = inputs.table(name, key_size)
            after = Table(payloads[body], payloads[header], key_size)
            if [(r.key, r.name) for r in before.records] != [(r.key, r.name) for r in after.records]:
                raise ValueError("Replacement lost/reordered game records")
            changed = [r.key for r in before.records if r.data != after.by_key[r.key].data]
            if changed != [target]:
                raise ValueError(f"Unexpected modified records: {changed}")
            if healthbar:
                source = before.require(91002, "Equip_Passive_ShowHPUI")
                actual = after.require(1201, "Passive_Player_BasicSkill")
                expected_payload = bytearray(source.data[source.prefix_size:])
                struct.pack_into("<I", expected_payload, 70 - actual.prefix_size, 1201)
                if actual.data[actual.prefix_size:] != expected_payload:
                    raise ValueError("Healthbar ability differs")
                if inputs.version == "2.04.02":
                    after.require(1000061, "Passive_Immune_BuffedActionRestriction")
                    after.require(15066, "Skill_Oongka_RocketPack_Start")
                if len(manifest["patches"]) != 3:
                    raise ValueError("Wrong character patch count")
            else:
                catalog = [tuple(map(int, line.split("\t")[:3]))
                           for line in (inputs.recipe_root / "alden_catalog.tsv").read_text(encoding="utf-8").splitlines()
                           if line.strip() and not line.startswith("#")]
                store = after.by_key[target].data
                if struct.unpack_from("<I", store, 49)[0] != 379 or struct.unpack_from("<I", store, 77)[0] != 379:
                    raise ValueError("Wrong Alden stock count")
                actual = [(struct.unpack_from("<I", store, 81 + i * 127 + 26)[0],
                           struct.unpack_from("<I", store, 81 + i * 127 + 43)[0],
                           struct.unpack_from("<I", store, 81 + i * 127 + 18)[0]) for i in range(379)]
                if actual != catalog or len(manifest["patches"]) != 502:
                    raise ValueError("Alden catalog or price count differs")
                for patch in manifest["patches"]:
                    if struct.unpack_from("<Q", bytes.fromhex(patch["patched"]), 4)[0] != 1:
                        raise ValueError("Alden price differs")
            replacement_checks.append(dict(package=package.name, records=len(after.records),
                                           changed_keys=changed, all_other_records_preserved=True))
    return dict(game_version=inputs.version, steam_build=inputs.steam_build,
                **result, replacement_checks=replacement_checks,
                manager_mount_tested=False, in_game_tested=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--game-version", choices=PROFILES, required=True)
    parser.add_argument("--expected-count", type=int, default=5)
    args = parser.parse_args()
    result = verify(Inputs(args.inputs, args.game_version), args.output, args.expected_count)
    write_json(args.output / "DATA_VALIDATION.json", result)
    print(f"PASS: {len(result['packages'])} data packages; all non-target records preserved; no patch conflicts")


if __name__ == "__main__":
    main()

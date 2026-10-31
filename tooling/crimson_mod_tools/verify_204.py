"""Verify the 2.04.00 source guards and composed data overlays, without running the game."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import zipfile

from .build_204 import ROOT, Inputs, VERSION, STEAM_BUILD, write_json
from .staticinfo import Table
from .native_204 import NATIVE_BUILD, NATIVE_EXE_SHA, NATIVE_VERSION

EXE_SHA = NATIVE_EXE_SHA


class Image:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        nt = struct.unpack_from("<I", self.data, 0x3c)[0]
        if self.data[:2] != b"MZ" or self.data[nt:nt+4] != b"PE\0\0":
            raise ValueError("Invalid PE image")
        machine, count = struct.unpack_from("<HH", self.data, nt+4)
        if machine != 0x8664:
            raise ValueError("Expected x64 PE image")
        optional = struct.unpack_from("<H", self.data, nt+20)[0]
        self.base = struct.unpack_from("<Q", self.data, nt+24+24)[0]
        self.sections = []
        for i in range(count):
            pos = nt + 24 + optional + i*40
            size, rva, raw_size, offset = struct.unpack_from("<IIII", self.data, pos+8)
            self.sections.append((rva, raw_size, offset))

    def read(self, rva, count):
        for start, size, offset in self.sections:
            if start <= rva and rva+count <= start+size:
                return self.data[offset+rva-start:offset+rva-start+count]
        raise ValueError(f"RVA not backed by file bytes: {rva:x}")


def guards(image):
    mining = ROOT / "mods/mining-helmet-always-on/source/src"
    bank = ROOT / "mods/faster-bank-refresh-gold-and-bonds/source/src"
    checked = []

    def check(label, rva, expected):
        if image.read(rva, len(expected)) != expected:
            raise ValueError(f"Native guard differs: {label} at {rva:x}")
        checked.append(dict(name=label, rva=hex(rva), bytes=expected.hex()))

    def byte_values(text):
        return bytes(int(x, 16) for x in re.findall(r"0x[0-9a-fA-F]+", text))

    for path, expected_count in ((mining / "material/build_guards.hpp", 15), (bank / "bank_refresh.cpp", 7)):
        source = path.read_text()
        symbols = {name: int(value, 16) for name, value in re.findall(r"(\w+Rva)\s*=\s*(0x[0-9a-fA-F]+)", source)}
        rows = re.findall(r"\{(0x[0-9a-fA-F]+|\w+Rva),\s*\{([^}]+)\}\}", source)
        if len(rows) != expected_count:
            raise ValueError(f"Unexpected guard count in {path}")
        for address, payload in rows:
            check(path.name, symbols[address] if address in symbols else int(address, 16), byte_values(payload))
    source = (mining / "runtime/group_range.hpp").read_text()
    check("ore group reader", int(re.search(r"ReaderRva=(0x\w+)", source)[1], 16),
          byte_values(re.search(r"ReaderHead=\{([^}]+)\}", source)[1]))
    runtime = (mining / "runtime/mining_helmet.cpp").read_text()
    for slot, target in re.findall(r"\{(0x[0-9a-f]+),(0x[0-9a-f]+),reinterpret_cast<void\*>\(", runtime):
        check("mining vtable slot", int(slot, 16), struct.pack("<Q", image.base+int(target, 16)))
    mounts = ROOT / "mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/variants"
    for path in sorted(mounts.rglob("*20402.c")):
        source = path.read_text()
        for field, pointer in (("guardBytes", "guardAt"), ("targetBytes", "g_target")):
            rva = int(re.search(rf"{pointer} = imageBase \+ (0x[0-9A-F]+)", source)[1], 16)
            check(path.name+":"+field, rva, byte_values(re.search(rf"{field}\[16\] = \{{([^}}]+)\}}", source)[1]))
    for path in (mining / "material/build_guards.hpp", bank / "bank_core.hpp"):
        if EXE_SHA not in path.read_text():
            raise ValueError(f"Wrong EXE hash in {path}")
    return checked


def data_packages(inputs, output, expected_count=5):
    packages = []
    all_patches = {}
    for path in sorted(output.glob("*.zip")):
        with zipfile.ZipFile(path) as z:
            if z.testzip():
                raise ValueError(f"Corrupt ZIP: {path.name}")
            manifests = [n for n in z.namelist() if n.endswith(f"/{path.stem}.json")]
            if not manifests:
                if any(n.lower().endswith(".json") for n in z.namelist()):
                    raise ValueError(f"Native package has a JSON file DMM would import as a mod: {path.name}")
                if any("/" in n for n in z.namelist() if n.lower().endswith(".asi")):
                    raise ValueError(f"Native plugin is not at the package root: {path.name}")
                continue
            if [n for n in z.namelist() if n.lower().endswith(".json")] != manifests:
                raise ValueError(f"Non-manifest JSON would become a phantom DMM mod: {path.name}")
            manifest = json.loads(z.read(manifests[0]))
            entries = []
            for patch in manifest["patches"]:
                target = patch["file"]
                original = inputs.read(Path("0008") / target)
                start = patch["offset"]
                before, after = bytes.fromhex(patch["original"]), bytes.fromhex(patch["patched"])
                if len(before) != len(after) or original[start:start+len(before)] != before:
                    raise ValueError(f"Patch input mismatch: {path.name}/{patch['description']}")
                entries.append((target, start, before, after))
                all_patches.setdefault((target, start, before, after), set()).add(path.stem)
            packages.append(dict(name=path.name, patches=len(entries), zip_readback=True))
    if len(packages) != expected_count:
        raise ValueError(f"Expected {expected_count} data packages, found {len(packages)}")
    # Identical healthbar alternatives are deduplicated. Any disagreement,
    # including a partial overlap with Alden/Steelheart, fails the build.
    edits = {}
    sources = {}
    for (target, start, before, after), owners in all_patches.items():
        changed = edits.setdefault(target, {})
        for index, value in enumerate(after, start):
            if index in changed and changed[index] != value:
                raise ValueError(f"Conflicting byte patches: {target}/{index}/{owners}")
            changed[index] = value
        sources[target] = inputs.read(Path("0008") / target)
    for target, offsets in edits.items():
        data = bytearray(sources[target])
        for index, value in offsets.items():
            data[index] = value
        header = inputs.read(Path("0008") / target.replace(".staticinfobody", ".staticinfoheader"))
        original, composed = Table(sources[target], header), Table(bytes(data), header)
        if [(r.key, r.name) for r in original.records] != [(r.key, r.name) for r in composed.records]:
            raise ValueError("Composed patches changed table identity")
    # Check the actual current indices used by the native plugin.
    for name, key, index, expected in (("effectinfo", 1001881, 2502, "fx_define_sign_b__mine1"),
                                      ("specialmode", 10, 13, "FindMine")):
        table = inputs.table(name)
        if table.records[index].key != key or table.records[index].name != expected:
            raise ValueError(f"Native table index changed: {name}")
    return dict(packages=packages, conflicting_patch_bytes=0, native_table_indices_verified=True,
                mutually_exclusive_variants=["three Healthbar presentations", "two Mount stat plugins"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--inputs", type=Path)
    parser.add_argument("--native-only", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    image = Image(args.exe)
    if hashlib.sha256(image.data).hexdigest() != EXE_SHA:
        raise ValueError(f"The installed EXE is not supported Steam build {NATIVE_BUILD}")
    if not args.native_only and args.inputs is None:
        parser.error("--inputs is required unless --native-only is selected")
    result = dict(game_version=NATIVE_VERSION, steam_build=NATIVE_BUILD, exe_sha256=EXE_SHA,
                  native_guards=guards(image), data_validation=None if args.native_only else data_packages(Inputs(args.inputs), args.output),
                  data_recipe_build=None if args.native_only else STEAM_BUILD,
                  in_game_tested=False, manager_mount_tested=False)
    write_json(args.output / "COMPATIBILITY_VALIDATION.json", result)
    print(f"PASS: {len(result['native_guards'])} native guard/slot checks; " +
          ("native-only verification" if args.native_only else "five data packages; no conflicting patch bytes"))


if __name__ == "__main__":
    main()

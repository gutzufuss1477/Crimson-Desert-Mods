"""Reproducible, fail-closed 2.04.x mod builds from locally extracted vanilla files.

No game assets are included in this module or its recipes. The input hashes
identify the selected Steam build; other inputs are refused.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile
from collections import Counter

from .staticinfo import Table, unique_offset
from .dmm_builder import healthbar_ui

VERSION = "2.04.02"
STEAM_BUILD = "26329202"
PROFILES = {"2.04.02": "26329202"}
ROOT = Path(__file__).resolve().parents[2]
BINARY = Path("0008/gamedata/binarystaticinfo__/bin")
UI = Path("0012/ui/xml/gamemain/play")


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def write_json(path: Path, value) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


class Inputs:
    def __init__(self, root: Path, version: str = VERSION):
        self.root = root.resolve()
        self.version = version
        self.steam_build = PROFILES[version]
        self.recipe_root = ROOT / "recipes" / version
        self.guards = json.loads((self.recipe_root / "input-hashes.json").read_text(encoding="utf-8"))
        self.used = {}

    def read(self, path: Path) -> bytes:
        name = path.as_posix()
        data = (self.root / path).read_bytes()
        digest = sha(data)
        if self.guards.get(name) != digest:
            raise ValueError(f"Not the supported vanilla {self.version} file: {name} ({digest})")
        self.used[name] = digest
        return data

    def table(self, name: str, key_size: int = 4) -> Table:
        return Table(self.read(BINARY / f"{name}.staticinfobody"),
                     self.read(BINARY / f"{name}.staticinfoheader"), key_size)


def patch(table: str, offset: int, original: bytes, replacement: bytes, description: str) -> dict:
    if not original or len(original) != len(replacement):
        raise ValueError("Invalid patch length")
    target = (BINARY / f"{table}.staticinfobody").relative_to("0008").as_posix()
    return dict(description=description, file=target, target=target, offset=offset,
                original=original.hex().upper(), patched=replacement.hex().upper())


def manifest(slug: str, description: str, patches: list[dict], version: str = VERSION) -> dict:
    return dict(name=slug, version=version, author="Blablup", description=description,
                patches=patches, dmm_offsetautorelocate_disable=True, dmm_v3upgrade_disable=True)


def healthbar(inputs: Inputs) -> tuple[dict[str, bytes], list[dict], dict]:
    characters = inputs.table("characterinfo")
    patches = []
    marker = bytes.fromhex("FBB0010001000000FA000000")
    for key, name in ((1, "Kliff"), (1002113, "Kliff_AI"), (1004085, "Yann")):
        r = characters.require(key, name)
        pos = unique_offset(r.data, marker) + 8
        patches.append(patch("characterinfo", r.start + pos, struct.pack("<I", 250),
                             struct.pack("<I", 91002), f"{name}: HP range helper -> Equip_Passive_ShowHPUI"))
    skills = inputs.table("skill")
    target = skills.require(1201, "Passive_Player_BasicSkill")
    source = skills.require(91002, "Equip_Passive_ShowHPUI")
    replacement = bytearray(target.data[:target.prefix_size] + source.data[source.prefix_size:])
    pos = unique_offset(replacement, struct.pack("<I", source.key))
    if pos != 70 or len(replacement) != 408:
        raise ValueError("HP skill payload/self-reference layout changed")
    struct.pack_into("<I", replacement, pos, target.key)
    rebuilt = skills.replace({target.key: bytes(replacement)})
    files = {
        (BINARY / "skill.staticinfobody").as_posix(): rebuilt.body,
        (BINARY / "skill.staticinfoheader").as_posix(): rebuilt.header,
    }
    return files, patches, dict(skill_records=len(skills.records), changed_skill_keys=[1201],
                               preserved_skill_records=len(skills.records) - 1,
                               character_offsets=[p["offset"] for p in patches])


def steelheart(inputs: Inputs) -> tuple[list[dict], dict]:
    items = inputs.table("iteminfo")
    item = items.require(1000594, "HorseShoe_HorseArmor_Shoe_III")
    buffs = inputs.table("buffinfo")
    buffs.require(1000046, "BuffLevel_StaminaRegen")
    buffs.require(1000213, "BuffLevel_HorseArmor_Shoe_StaminaRegen")
    pos = unique_offset(item.data, struct.pack("<II", 1000046, 4))
    if pos != 369 or len(item.data) != 604:
        raise ValueError("Steelheart equip-buff layout changed")
    patches = [patch("iteminfo", item.start + pos, struct.pack("<II", 1000046, 4),
                     struct.pack("<II", 1000046, 20), "Steelheart stamina regeneration equip-buff: level 4 -> 20")]
    return patches, dict(item_key=item.key, record_relative_offset=pos, old_level=4, new_level=20)


def alden(inputs: Inputs) -> tuple[dict[str, bytes], list[dict], dict]:
    catalog = []
    for line in (inputs.recipe_root / "alden_catalog.tsv").read_text(encoding="utf-8").splitlines():
        if line.strip() and not line.startswith("#"):
            catalog.append(tuple(map(int, line.split("\t")[:3])))
    if ([row[0] for row in catalog] != list(range(379)) or
            len({row[1] for row in catalog}) != 379 or
            Counter(row[2] for row in catalog) != {1: 258, 99: 93, 999: 28}):
        raise ValueError("The original ordered Alden catalog changed")
    items = inputs.table("iteminfo")
    price_recipe = json.loads((inputs.recipe_root / "alden-prices.json").read_text(encoding="utf-8"))["items"]
    if [r["key"] for r in price_recipe] != [row[1] for row in catalog]:
        raise ValueError("Price recipe/catalog mismatch")
    patches = []
    for recipe in price_recipe:
        item = items.require(recipe["key"], recipe["name"])
        if sha(item.data) != recipe["record_sha256"]:
            raise ValueError(f"Alden price record changed: {item.name}")
        for price in recipe["prices"]:
            pos = price["offset"]
            before = struct.pack("<IQII", price["currency"], price["original"], 0, price["currency"])
            after = struct.pack("<IQII", price["currency"], 1, 0, price["currency"])
            if item.data[pos - 4:pos + 16] != before:
                raise ValueError(f"Alden PriceFloor context changed: {item.name}/{pos}")
            patches.append(patch("iteminfo", item.start + pos - 4, before, after,
                                 f"{item.name}: purchase price in currency {price['currency']} -> 1"))
    stores = inputs.table("storeinfo", 2)
    store = stores.require(3101, "Store_Her_General")
    data = store.data
    if len(data) != 5314 or struct.unpack_from("<I", data, 49)[0] != 40 or struct.unpack_from("<I", data, 77)[0] != 40:
        raise ValueError("Alden stock count layout changed")
    starts = []
    marker = struct.pack("<HQQ", 3101, 1000000, 1000000)
    pos = 0
    while (pos := data.find(marker, pos)) >= 0:
        starts.append(pos)
        pos += 1
    if len(starts) != 40 or starts[0] != 81 or data[-10:] != bytes.fromhex("00000000020000003d3e"):
        raise ValueError("Alden stock boundaries/trailer changed")
    entries = [data[a:b] for a, b in zip(starts, starts[1:] + [len(data) - 10], strict=True)]
    if any(len(e) not in (127, 140) or e[43:47] != e[102:106] for e in entries):
        raise ValueError("Alden stock entry layout changed")
    # A current vanilla row with no restore flag or stock condition. Copy all
    # other native defaults, including the added eight-byte refresh condition.
    template = next(e for e in entries if len(e) == 127 and e[41] == 0 and e[114] == 0)
    if template[115:] != bytes(12):
        raise ValueError("Alden template has a refresh/effect restriction")
    rows = []
    for index, key, stock in catalog:
        if key not in items.by_key:
            raise ValueError(f"Missing catalog item: {key}")
        row = bytearray(template)
        for offset, value in ((18, stock), (26, index), (30, index), (43, key), (102, key)):
            struct.pack_into("<I", row, offset, value)
        rows.append(bytes(row))
    prefix = bytearray(data[:81])
    for offset in (49, 77):
        struct.pack_into("<I", prefix, offset, len(rows))
    rebuilt = stores.replace({3101: bytes(prefix) + b"".join(rows) + data[-10:]})
    # Decode the actual assembled rows independently of the row-construction loop.
    payload = rebuilt.by_key[3101].data
    actual = [(struct.unpack_from("<I", payload, 81 + i * 127 + 26)[0],
               struct.unpack_from("<I", payload, 81 + i * 127 + 43)[0],
               struct.unpack_from("<I", payload, 81 + i * 127 + 18)[0]) for i in range(379)]
    if actual != catalog or len(payload) != 81 + 379 * 127 + 10:
        raise ValueError("Alden rebuilt catalog failed round-trip")
    files = {(BINARY / "storeinfo.staticinfobody").as_posix(): rebuilt.body,
             (BINARY / "storeinfo.staticinfoheader").as_posix(): rebuilt.header}
    return files, patches, dict(catalog_items=379, price_entries=len(patches),
                               store_records=len(stores.records), changed_store_keys=[3101],
                               preserved_store_records=len(stores.records) - 1)


def package(output: Path, slug: str, description: str, patches: list[dict], files: dict[str, bytes],
            inputs: Inputs, validation: dict) -> Path:
    payloads = {f"files/{name}": data for name, data in files.items()}
    recipe = manifest(slug, description, patches, inputs.version)
    payloads[f"{slug}.json"] = (json.dumps(recipe, indent=2) + "\n").encode()
    report = dict(game_version=inputs.version, steam_build=inputs.steam_build, input_sha256=inputs.used.copy(),
                  validation=validation, runtime_tested=False, manager_mount_tested=False)
    # DMM treats every imported .json as an executable mod manifest.
    # Keep diagnostic JSON content in a text file so it cannot become a mod.
    payloads["BUILD_VALIDATION.txt"] = (json.dumps(report, indent=2) + "\n").encode()
    payloads["README.txt"] = (
        f"{slug}\n\n{description}\n\n"
        f"Built against Crimson Desert {inputs.version}, Steam build {inputs.steam_build}.\n"
        "Replace the previous version; do not enable old and new versions together.\n"
        "Healthbar: enable only one presentation variant.\n"
        "This package uses the new binarystaticinfo paths and StaticInfo format.\n"
        "Use DMM 4.5.1 or a newer compatible version. Import this ZIP through DMM.\n"
        "Do not copy the files/ folders directly into the game directory.\n"
        "DMM 4.4.2 cannot mount the 2.04.x archive format.\n"
        "Close the game before replacing a mounted package, then mount again\n"
        "and restart the game so the runtime loads the new files.\n"
        "Package/data validation passed. DMM mount and in-game tests are pending.\n"
    ).encode()
    output.mkdir(parents=True, exist_ok=True)
    destination = output / f"{slug}.zip"
    with zipfile.ZipFile(destination, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, data in sorted(payloads.items()):
            info = zipfile.ZipInfo(f"{slug}/{name}", (2026, 10, 31 if inputs.version == "2.04.02" else 30, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, data, compresslevel=9)
    with zipfile.ZipFile(destination) as z:
        if z.testzip() is not None:
            raise ValueError("ZIP round-trip failed")
        for name, data in payloads.items():
            if z.read(f"{slug}/{name}") != data:
                raise ValueError("ZIP payload changed")
    write_json(inputs.recipe_root / slug / "manifest.json", recipe)
    write_json(output / f"{slug}.validation.json", report | {"package_sha256": sha(destination.read_bytes())})
    return destination


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", type=Path, required=True, help="Extracted vanilla root containing 0008 and 0012")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--game-version", choices=PROFILES, default=VERSION)
    parser.add_argument("--mods", nargs="+", choices=["healthbar", "steelheart", "alden"], default=["healthbar", "steelheart", "alden"])
    args = parser.parse_args()
    version = args.game_version
    if "healthbar" in args.mods:
        inputs = Inputs(args.inputs, version)
        files, patches, validation = healthbar(inputs)
        for extension in ("html", "css"):
            inputs.read(UI / f"subtitletagview.{extension}")
        for variant, label in (("multi", "vanilla_multitarget"), ("classic", "classic_vanilla_single_target"), ("caites", "Caites")):
            html, css = healthbar_ui(inputs.root, variant)
            variant_files = files | {(UI / "subtitletagview.html").as_posix(): html,
                                     (UI / "subtitletagview.css").as_posix(): css}
            description = f"Persistent enemy HP bars ({label}) for Crimson Desert {version}."
            style = "compact thin red bars" if variant == "caites" else "classic red bars with dark frame"
            behavior = "single target; no native multitarget patch" if variant == "classic" else "multiple targets; requires NativeFix2 ASI helper"
            description += f" {style}; {behavior}. Use package_healthbar_204 to include the helper in a DMM import ZIP."
            variant_validation = validation | {"ui_revision": "VisualFix3", "show_hp_buff_tag_preserved": True,
                "style": style, "target_mode": "single" if variant == "classic" else "multi",
                "requires_native_helper": variant != "classic", "visual_gameplay_tested": False}
            print(package(args.output, f"Healthbar_always_on_{label}_{version}",
                          description, patches, variant_files, inputs, variant_validation))
    if "steelheart" in args.mods:
        inputs = Inputs(args.inputs, version)
        patches, validation = steelheart(inputs)
        print(package(args.output, f"Steelheart_Horseshoes_20_Stamina_Regen_{version}",
                      "Sets the Steelheart Horseshoes stamina-regeneration equip-buff level from 4 to 20.",
                      patches, {}, inputs, validation))
    if "alden" in args.mods:
        inputs = Inputs(args.inputs, version)
        files, patches, validation = alden(inputs)
        print(package(args.output, f"Alden_AIO_Shop_All_Items_1_Copper_{version}",
                      "The original 379-item Alden catalog, order and stock. All catalog purchase prices are set to 1.",
                      patches, files, inputs, validation))


if __name__ == "__main__":
    main()

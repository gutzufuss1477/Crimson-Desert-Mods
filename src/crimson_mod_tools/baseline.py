from __future__ import annotations

import json
import shutil
import struct
from pathlib import Path
from typing import Any

from .builders import _locate_store_entries
from .common import REQUIRED_GAME_FILES, game_file_hashes, read_json, write_json
from .locators import ContextLocator, locate_context
from .paz import read_overlay_file
from .recipes import RecipeSet
from .records import parse_store_header, parse_u32_header


def _unique_context(record: bytes, relative: int, size: int, *, minimum: int = 8, maximum: int = 48) -> tuple[str, str]:
    for context_size in range(minimum, maximum + 1, 4):
        before_start = max(0, relative - context_size)
        after_end = min(len(record), relative + size + context_size)
        before = record[before_start:relative]
        after = record[relative + size:after_end]
        matches: list[int] = []
        cursor = 0
        while True:
            prefix = record.find(before, cursor)
            if prefix < 0:
                break
            value_start = prefix + len(before)
            if record[value_start + size:value_start + size + len(after)] == after:
                matches.append(value_start)
            cursor = prefix + 1
        if matches == [relative]:
            return before.hex().upper(), after.hex().upper()
    raise RuntimeError(f"Could not create a unique context at relative offset {relative}")


def _find_one(root: Path, pattern: str) -> Path:
    matches = list(root.glob(pattern))
    if len(matches) != 1:
        raise RuntimeError(f"Expected one match for {pattern} below {root}, found {matches}")
    return matches[0]


def capture_baseline(
    *,
    game_root: Path,
    built_output: Path,
    previous: RecipeSet,
    destination: Path,
    game_version: str,
    force: bool = False,
) -> dict[str, Any]:
    if destination.exists():
        if not force:
            raise RuntimeError(f"Recipe destination already exists: {destination}")
        shutil.rmtree(destination)
    destination.mkdir(parents=True)

    write_json(
        destination / "baseline.json",
        {
            "game_version": game_version,
            "created_from": "User-confirmed in-game working build",
            "files": game_file_hashes(game_root),
        },
    )
    write_json(destination / "mounts.json", previous.mounts)
    write_json(destination / "steelheart.json", previous.steelheart)

    binary = game_root / "0008/gamedata/binary__/client/bin"
    item_data = (binary / "iteminfo.pabgb").read_bytes()
    item_records = parse_u32_header(item_data, (binary / "iteminfo.pabgh").read_bytes())
    items_by_key = {record.key: record for record in item_records}

    alden_folder = _find_one(built_output / "normal", "Alden_AIO_Shop_plus_All_Items_1_Copper_*")
    alden_json = read_json(alden_folder / "Alden_AIO_Shop_plus_All_Items_1_Copper.json")
    built_price_changes = {
        int(change["offset"]): change
        for change in alden_json["patches"][0]["changes"]
    }
    price_recipes: list[dict[str, Any]] = []
    for previous_patch in previous.alden_prices["patches"]:
        item_id = int(previous_patch["item_id"])
        record = items_by_key[item_id]
        payload = item_data[record.start:record.end]
        relative = locate_context(payload, ContextLocator.from_recipe(previous_patch))
        absolute = record.start + relative
        current = payload[relative:relative + 4]
        target = bytes.fromhex(previous_patch.get("target", "01000000"))
        built_change = built_price_changes.get(absolute)
        if built_change is None:
            if current != target:
                raise RuntimeError(
                    f"Alden price {item_id} was neither patched nor already at the target value"
                )
        else:
            if bytes.fromhex(built_change["original"].replace(" ", "")) != current:
                raise RuntimeError(f"Alden price original-byte mismatch for {item_id}")
            if bytes.fromhex(built_change["patched"].replace(" ", "")) != target:
                raise RuntimeError(f"Alden price target-byte mismatch for {item_id}")
        before, after = _unique_context(payload, relative, 4)
        price_recipes.append(
            {
                **previous_patch,
                "item_name": record.name,
                "relative_offset": relative,
                "before": before,
                "after": after,
                "expected": current.hex().upper(),
                "target": target.hex().upper(),
            }
        )
    complete_prices = sorted(price_recipes, key=lambda value: (int(value["item_id"]), int(value["relative_offset"])))
    if len(complete_prices) != len(previous.alden_prices["patches"]):
        raise RuntimeError("Captured Alden price recipe count changed")
    write_json(destination / "alden_prices.json", {"target_value": 1, "patches": complete_prices})

    store_data = (binary / "storeinfo.pabgb").read_bytes()
    store_header = (binary / "storeinfo.pabgh").read_bytes()
    store_records = parse_store_header(store_data, store_header)
    store_key = int(previous.alden_store["store_key"])
    store_record = next(record for record in store_records if record.key == store_key)
    vanilla_target = store_data[store_record.start:store_record.end]
    id_offsets = tuple(int(value) for value in previous.alden_store["item_id_offsets"])
    trailer_size = int(previous.alden_store["trailer_size"])
    starts, vanilla_entries, _ = _locate_store_entries(vanilla_target, store_key, id_offsets, trailer_size)
    list_start = starts[0]

    count_locators: list[dict[str, Any]] = []
    for prior_locator in previous.alden_store["count_locators"]:
        old_locator = ContextLocator.from_recipe(prior_locator)
        relative = locate_context(vanilla_target, old_locator)
        if relative >= list_start:
            raise RuntimeError("Captured Alden count field resolved inside the entry list")
        expected = vanilla_target[relative:relative + 4]
        if struct.unpack("<I", expected)[0] != len(vanilla_entries):
            raise RuntimeError("Captured Alden count field does not match parsed vanilla entry count")
        before, after = _unique_context(vanilla_target, relative, 4)
        count_locators.append(
            {
                "relative_offset": relative,
                "before": before,
                "after": after,
                "expected": expected.hex().upper(),
            }
        )

    pamt = alden_folder / "0036/0.pamt"
    paz = alden_folder / "0036/0.paz"
    mod_store_data = read_overlay_file(pamt, paz, "storeinfo.pabgb")
    mod_store_header = read_overlay_file(pamt, paz, "storeinfo.pabgh")
    mod_records = parse_store_header(mod_store_data, mod_store_header)
    mod_record = next(record for record in mod_records if record.key == store_key)
    mod_target = mod_store_data[mod_record.start:mod_record.end]
    _, mod_entries, _ = _locate_store_entries(mod_target, store_key, id_offsets, trailer_size)
    entry_size = int(previous.alden_store["entry_size"])
    entries: list[dict[str, Any]] = []
    for item_id, payload in mod_entries:
        if len(payload) != entry_size:
            raise RuntimeError(f"Captured Alden entry size changed for {item_id}: {len(payload)}")
        entries.append(
            {
                "item_id": item_id,
                "slice_18_22": payload[18:22].hex().upper(),
                "slice_26_118": payload[26:118].hex().upper(),
            }
        )
    write_json(
        destination / "alden_store.json",
        {
            **{key: previous.alden_store[key] for key in ("store_key", "store_name", "template_item_id", "entry_size", "item_id_offsets", "trailer_size")},
            "count_locators": count_locators,
            "entries": entries,
            "baseline_vanilla_offer_count": len(vanilla_entries),
            "baseline_mod_offer_count": len(entries),
        },
    )

    character_data = (binary / "characterinfo.pabgb").read_bytes()
    character_records = parse_u32_header(character_data, (binary / "characterinfo.pabgh").read_bytes())
    characters_by_key = {record.key: record for record in character_records}
    healthbar_folder = _find_one(built_output / "normal", "Healthbar_always_on_classic_vanilla_single_target_*")
    healthbar_json = read_json(healthbar_folder / "Healthbar_always_on.json")
    patches_by_description = {patch["description"]: patch for patch in healthbar_json["patches"]}
    characters: list[dict[str, Any]] = []
    for previous_character in previous.healthbar["characters"]:
        key = int(previous_character["key"])
        record = characters_by_key[key]
        description = f"Enable Equip_Passive_ShowHPUI for {record.name} ({record.key})."
        patch = patches_by_description[description]
        relative = int(patch["offset"]) - record.start
        payload = character_data[record.start:record.end]
        before, after = _unique_context(payload, relative, 4)
        characters.append(
            {
                "key": key,
                "name": record.name,
                "relative_offset": relative,
                "before": before,
                "after": after,
                "expected": payload[relative:relative + 4].hex().upper(),
                "patched": patch["patched"].replace(" ", "").upper(),
            }
        )
    write_json(
        destination / "healthbar.json",
        {
            **{key: previous.healthbar[key] for key in ("skill_target_key", "skill_target_name", "skill_source_key", "skill_source_name", "ui", "variants")},
            "characters": characters,
        },
    )

    return {
        "game_version": game_version,
        "destination": str(destination),
        "alden_prices": len(complete_prices),
        "alden_store_entries": len(entries),
        "healthbar_characters": len(characters),
    }

from __future__ import annotations

import re
import struct
import zipfile
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .common import (
    compact_hex,
    ensure_unique,
    find_all,
    reset_directory,
    sha256_file,
    spaced_hex,
    write_json,
    zip_directory,
)
from .locators import ContextLocator, locate_context
from .paz import build_two_file_overlay
from .recipes import RecipeSet
from .records import (
    Record,
    build_store_header,
    build_u32_header,
    parse_store_header,
    parse_u32_header,
)


@dataclass
class BuildContext:
    repo_root: Path
    game_root: Path
    game_version: str
    output_root: Path
    recipes: RecipeSet

    @property
    def binary_root(self) -> Path:
        return self.game_root / "0008/gamedata/binary__/client/bin"

    @property
    def ui_root(self) -> Path:
        return self.game_root / "0012/ui/xml/gamemain/play"

    @property
    def normal_root(self) -> Path:
        return self.output_root / "normal"

    @property
    def dmm_root(self) -> Path:
        return self.output_root / "dmm"


@dataclass
class BuildResult:
    name: str
    status: str
    packages: list[Path]
    details: dict[str, Any]
    error: str | None = None


def _simple_json_mod(
    folder: Path,
    json_name: str,
    payload: dict[str, Any],
    readme: str,
) -> None:
    folder.mkdir(parents=True, exist_ok=True)
    write_json(folder / json_name, payload)
    (folder / "README.txt").write_text(readme, encoding="utf-8", newline="\n")


def _package(folder: Path, packages_root: Path) -> Path:
    destination = packages_root / f"{folder.name}.zip"
    zip_directory(folder, destination, include_root=True)
    return destination


def build_mounts(context: BuildContext) -> BuildResult:
    data_path = context.binary_root / "characterinfo.pabgb"
    header_path = context.binary_root / "characterinfo.pabgh"
    data = data_path.read_bytes()
    records = parse_u32_header(data, header_path.read_bytes())
    by_name: dict[str, list[Record]] = defaultdict(list)
    for record in records:
        by_name[record.name].append(record)

    targets = list(context.recipes.mounts["target_records"])
    missing = [name for name in targets if len(by_name[name]) != 1]
    if missing:
        raise RuntimeError(f"Mount records missing or non-unique: {missing[:20]}")

    target_level = int(context.recipes.mounts.get("target_level", 5))
    fields = list(context.recipes.mounts.get("fields", ["movement speed", "acceleration", "turning", "jump"]))
    speed_changes: list[dict[str, Any]] = []
    stats_changes: list[dict[str, Any]] = []
    block_counts: dict[str, int] = {}

    for name in targets:
        record = by_name[name][0]
        payload = data[record.start:record.end]
        blocks: list[int] = []
        for relative in range(0, len(payload) - 15):
            values = struct.unpack_from("<IIII", payload, relative)
            if all(1 <= value <= 5 for value in values):
                if blocks and relative < blocks[-1] + 16:
                    continue
                blocks.append(relative)
        block_counts[name] = len(blocks)
        for block_index, relative in enumerate(blocks, start=1):
            values = struct.unpack_from("<IIII", payload, relative)
            if values[0] != target_level:
                absolute = record.start + relative
                speed_changes.append(
                    {
                        "offset": absolute,
                        "label": f"{name} record-based block {block_index}: movement speed {values[0]} -> {target_level}, {context.game_version} semantic remap",
                        "original": compact_hex(struct.pack("<I", values[0])),
                        "patched": compact_hex(struct.pack("<I", target_level)),
                    }
                )
            for field_index, value in enumerate(values):
                if value == target_level:
                    continue
                absolute = record.start + relative + field_index * 4
                stats_changes.append(
                    {
                        "offset": absolute,
                        "label": f"{name} record-based block {block_index}: {fields[field_index]} {value} -> {target_level}, {context.game_version} semantic remap",
                        "original": compact_hex(struct.pack("<I", value)),
                        "patched": compact_hex(struct.pack("<I", target_level)),
                    }
                )

    ensure_unique((int(change["offset"]) for change in speed_changes), "mount speed offsets")
    ensure_unique((int(change["offset"]) for change in stats_changes), "mount stat offsets")
    packages_root = context.normal_root / "packages"

    speed_folder = context.normal_root / f"All_Mounts_LvL_5_Speed_{context.game_version}"
    _simple_json_mod(
        speed_folder,
        "All_Mounts_LvL_5_Speed.json",
        {
            "name": "All Mounts LvL 5 Speed",
            "version": context.game_version,
            "author": "Blablup",
            "description": f"Sets the movement-speed rating field in compatible mount stat blocks to level 5 for Crimson Desert {context.game_version}.",
            "patches": [{"game_file": "gamedata/characterinfo.pabgb", "changes": speed_changes}],
        },
        f"All Mounts LvL 5 Speed - Crimson Desert {context.game_version}\n\n"
        "Sets the movement-speed rating to level 5 in every detected compatible mount stat block.\n"
        "Do not combine this package with the All Stats variant.\n",
    )

    stats_folder = context.normal_root / f"All_Mounts_LvL_5_All_Stats_{context.game_version}"
    _simple_json_mod(
        stats_folder,
        "All_Mounts_LvL_5_All_Stats.json",
        {
            "name": "All Mounts LvL 5 All Stats",
            "version": context.game_version,
            "author": "Blablup",
            "description": f"Sets movement speed, acceleration, turning and jump ratings to level 5 for Crimson Desert {context.game_version}.",
            "patches": [{"game_file": "gamedata/characterinfo.pabgb", "changes": stats_changes}],
        },
        f"All Mounts LvL 5 All Stats - Crimson Desert {context.game_version}\n\n"
        "Sets movement speed, acceleration, turning and jump ratings to level 5.\n"
        "Do not combine this package with the Speed-only variant.\n",
    )

    packages = [_package(speed_folder, packages_root), _package(stats_folder, packages_root)]
    blocks = sum(block_counts.values())
    warnings: list[str] = []
    baseline_blocks = int(context.recipes.mounts.get("baseline_stat_blocks", blocks))
    if blocks != baseline_blocks:
        warnings.append(f"Detected {blocks} stat blocks; baseline had {baseline_blocks}.")
    return BuildResult(
        name="mounts",
        status="ok" if not warnings else "ok_with_warnings",
        packages=packages,
        details={
            "records": len(targets),
            "stat_blocks": blocks,
            "speed_changes": len(speed_changes),
            "all_stats_changes": len(stats_changes),
            "warnings": warnings,
        },
    )


def build_steelheart(context: BuildContext) -> BuildResult:
    recipe = context.recipes.steelheart
    data = (context.binary_root / "iteminfo.pabgb").read_bytes()
    records = parse_u32_header(data, (context.binary_root / "iteminfo.pabgh").read_bytes())
    candidates = [record for record in records if record.key == int(recipe["item_key"])]
    if len(candidates) != 1:
        raise RuntimeError("Steelheart item record is missing or duplicated")
    record = candidates[0]
    if record.name != recipe["item_name"]:
        raise RuntimeError(f"Steelheart item name changed: {record.name}")
    payload = data[record.start:record.end]
    effect_key = struct.pack("<I", int(recipe["effect_key"]))
    value_positions = [position + 4 for position in find_all(payload, effect_key) if position + 8 <= len(payload)]
    if len(value_positions) != 1:
        raise RuntimeError(f"Steelheart effect is not unique: {value_positions}")
    relative = value_positions[0]
    current = struct.unpack_from("<I", payload, relative)[0]
    target = int(recipe["target_value"])
    changes: list[dict[str, Any]] = []
    if current != target:
        changes.append(
            {
                "offset": record.start + relative,
                "label": f"Steelheart Horseshoes item {record.key} effect {recipe['effect_key']}: {current} -> {target}, {context.game_version} semantic remap",
                "original": compact_hex(struct.pack("<I", current)),
                "patched": compact_hex(struct.pack("<I", target)),
            }
        )
    folder = context.normal_root / f"Steelheart_Horseshoes_20_Stamina_Regen_{context.game_version}"
    _simple_json_mod(
        folder,
        "Steelheart_Horseshoes_20_Stamina_Regen.json",
        {
            "name": "Steelheart Horseshoes +20 Stamina Regen",
            "version": context.game_version,
            "author": "Blablup",
            "description": f"Changes the Steelheart Horseshoes stamina-regeneration value to +20 for Crimson Desert {context.game_version}.",
            "patches": [{"game_file": "gamedata/iteminfo.pabgb", "changes": changes}],
        },
        f"Steelheart Horseshoes +20 Stamina Regen - Crimson Desert {context.game_version}\n\n"
        f"Detected current value: {current}. Target value: {target}.\n",
    )
    package = _package(folder, context.normal_root / "packages")
    return BuildResult(
        name="steelheart",
        status="ok",
        packages=[package],
        details={"item_key": record.key, "relative_offset": relative, "current_value": current, "changes": len(changes)},
    )


def _locate_alden_price(record_payload: bytes, patch_recipe: dict[str, Any]) -> int:
    locator = ContextLocator.from_recipe(patch_recipe)
    return locate_context(record_payload, locator)


def _build_alden_prices(context: BuildContext) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    data = (context.binary_root / "iteminfo.pabgb").read_bytes()
    records = parse_u32_header(data, (context.binary_root / "iteminfo.pabgh").read_bytes())
    by_key = {record.key: record for record in records}
    changes: list[dict[str, Any]] = []
    already_target = 0
    relocated = 0
    for patch_recipe in context.recipes.alden_prices["patches"]:
        item_id = int(patch_recipe["item_id"])
        if item_id not in by_key:
            raise RuntimeError(f"Alden item {item_id} is missing")
        record = by_key[item_id]
        payload = data[record.start:record.end]
        relative = _locate_alden_price(payload, patch_recipe)
        if relative != int(patch_recipe["relative_offset"]):
            relocated += 1
        current = payload[relative:relative + 4]
        target = bytes.fromhex(patch_recipe.get("target", "01000000"))
        if current == target:
            already_target += 1
            continue
        changes.append(
            {
                "offset": record.start + relative,
                "label": f"Alden item {record.name} ({item_id}) price {struct.unpack('<I', current)[0]} -> 1, {context.game_version} semantic remap",
                "original": compact_hex(current),
                "patched": compact_hex(target),
            }
        )
    ensure_unique((int(change["offset"]) for change in changes), "Alden price offsets")
    return changes, {"changes": len(changes), "already_one": already_target, "relocated": relocated}


def _locate_store_entries(target: bytes, store_key: int, id_offsets: tuple[int, int], trailer_size: int) -> tuple[list[int], list[tuple[int, bytes]], bytes]:
    starts: list[int] = []
    key_bytes = struct.pack("<H", store_key)
    max_id_offset = max(id_offsets)
    for position in find_all(target, key_bytes):
        if position + max_id_offset + 4 > len(target):
            continue
        first_id = struct.unpack_from("<I", target, position + id_offsets[0])[0]
        second_id = struct.unpack_from("<I", target, position + id_offsets[1])[0]
        if first_id == second_id and first_id > 0:
            starts.append(position)
    starts = sorted(set(starts))
    if not starts:
        raise RuntimeError("No store entries detected")
    trailer = target[-trailer_size:]
    entries: list[tuple[int, bytes]] = []
    for index, position in enumerate(starts):
        end = starts[index + 1] if index + 1 < len(starts) else len(target) - trailer_size
        if end <= position:
            raise RuntimeError("Invalid store entry boundary")
        payload = target[position:end]
        item_id = struct.unpack_from("<I", payload, id_offsets[0])[0]
        entries.append((item_id, payload))
    return starts, entries, trailer


def _rebuild_alden_store(context: BuildContext) -> tuple[bytes, bytes, dict[str, Any]]:
    recipe = context.recipes.alden_store
    store_data = (context.binary_root / "storeinfo.pabgb").read_bytes()
    store_header = (context.binary_root / "storeinfo.pabgh").read_bytes()
    records = parse_store_header(store_data, store_header)
    store_key = int(recipe["store_key"])
    targets = [record for record in records if record.key == store_key]
    if len(targets) != 1:
        raise RuntimeError("Alden target store is missing or duplicated")
    target_record = targets[0]
    if target_record.name != recipe["store_name"]:
        raise RuntimeError(f"Alden target store name changed: {target_record.name}")
    target = store_data[target_record.start:target_record.end]
    id_offsets = tuple(int(value) for value in recipe["item_id_offsets"])
    trailer_size = int(recipe["trailer_size"])
    starts, vanilla_entries, trailer = _locate_store_entries(target, store_key, id_offsets, trailer_size)
    list_start = starts[0]

    template_item_id = int(recipe["template_item_id"])
    entry_size = int(recipe["entry_size"])
    templates = [payload for item_id, payload in vanilla_entries if item_id == template_item_id and len(payload) == entry_size]
    if len(templates) != 1:
        shortest = min(len(payload) for _, payload in vanilla_entries)
        templates = [payload for _, payload in vanilla_entries if len(payload) == shortest]
    if len(templates) != 1 or len(templates[0]) != entry_size:
        raise RuntimeError(
            f"Alden store schema changed: expected one {entry_size}-byte template, found {[len(value) for value in templates]}"
        )
    template = templates[0]

    rebuilt_entries: list[bytes] = []
    item_ids: list[int] = []
    for entry_recipe in recipe["entries"]:
        item_id = int(entry_recipe["item_id"])
        payload = bytearray(template)
        payload[18:22] = bytes.fromhex(entry_recipe["slice_18_22"])
        payload[26:118] = bytes.fromhex(entry_recipe["slice_26_118"])
        first_id = struct.unpack_from("<I", payload, id_offsets[0])[0]
        second_id = struct.unpack_from("<I", payload, id_offsets[1])[0]
        if first_id != item_id or second_id != item_id:
            raise RuntimeError(f"Alden item mapping failed for {item_id}: {first_id}/{second_id}")
        rebuilt_entries.append(bytes(payload))
        item_ids.append(item_id)
    ensure_unique(item_ids, "Alden store item IDs")

    header = bytearray(target[:list_start])
    old_count = len(vanilla_entries)
    count_offsets: list[int] = []
    for locator_recipe in recipe["count_locators"]:
        locator = ContextLocator.from_recipe(locator_recipe)
        offset = locate_context(target, locator)
        if offset >= list_start:
            raise RuntimeError("Alden list count locator resolved inside the entry list")
        current = struct.unpack_from("<I", header, offset)[0]
        if current != old_count:
            raise RuntimeError(f"Alden list count locator found {current}, parsed {old_count}")
        struct.pack_into("<I", header, offset, len(rebuilt_entries))
        count_offsets.append(offset)
    ensure_unique(count_offsets, "Alden list count offsets")
    replacement = bytes(header) + b"".join(rebuilt_entries) + trailer

    parts: list[bytes] = []
    offsets: list[int] = []
    cursor = 0
    for record in records:
        offsets.append(cursor)
        part = replacement if record.key == store_key else store_data[record.start:record.end]
        parts.append(part)
        cursor += len(part)
    rebuilt_data = b"".join(parts)
    rebuilt_header = build_store_header(records, offsets)

    parsed = parse_store_header(rebuilt_data, rebuilt_header)
    rebuilt_target_record = next(record for record in parsed if record.key == store_key)
    rebuilt_target = rebuilt_data[rebuilt_target_record.start:rebuilt_target_record.end]
    _, parsed_entries, parsed_trailer = _locate_store_entries(rebuilt_target, store_key, id_offsets, trailer_size)
    parsed_ids = [item_id for item_id, _ in parsed_entries]
    if parsed_ids != item_ids or parsed_trailer != trailer:
        raise RuntimeError("Alden rebuilt shop validation failed")
    for count_offset in count_offsets:
        if struct.unpack_from("<I", rebuilt_target, count_offset)[0] != len(item_ids):
            raise RuntimeError("Alden rebuilt count field is invalid")

    item_keys = {
        record.key
        for record in parse_u32_header(
            (context.binary_root / "iteminfo.pabgb").read_bytes(),
            (context.binary_root / "iteminfo.pabgh").read_bytes(),
        )
    }
    missing_items = sorted(set(item_ids) - item_keys)
    if missing_items:
        raise RuntimeError(f"Alden shop contains missing item IDs: {missing_items[:20]}")

    return rebuilt_data, rebuilt_header, {
        "vanilla_offer_count": old_count,
        "rebuilt_offer_count": len(item_ids),
        "count_offsets": count_offsets,
        "entry_size": entry_size,
        "records_preserved": len(records) - 1,
    }


def build_alden(context: BuildContext) -> BuildResult:
    prices, price_meta = _build_alden_prices(context)
    store_data, store_header, store_meta = _rebuild_alden_store(context)
    template = (context.repo_root / "assets/alden/0.pamt.template").read_bytes()
    pamt, paz = build_two_file_overlay(template, store_data, store_header)

    folder = context.normal_root / f"Alden_AIO_Shop_plus_All_Items_1_Copper_{context.game_version}"
    (folder / "0036").mkdir(parents=True, exist_ok=True)
    (folder / "0036/0.pamt").write_bytes(pamt)
    (folder / "0036/0.paz").write_bytes(paz)
    write_json(
        folder / "Alden_AIO_Shop_plus_All_Items_1_Copper.json",
        {
            "name": "Alden AIO Shop + All Items 1 Copper",
            "version": context.game_version,
            "author": "Blablup",
            "description": f"Alden shop with 379 items and 1-copper prices, rebuilt for Crimson Desert {context.game_version}.",
            "patches": [{"game_file": "gamedata/iteminfo.pabgb", "changes": prices}],
        },
    )
    write_json(
        folder / "modinfo.json",
        {
            "id": f"alden_aio_shop_all_items_1_copper_{context.game_version.replace('.', '_')}",
            "name": "Alden AIO Shop + All Items 1 Copper",
            "version": context.game_version,
            "author": "Blablup",
            "description": "Alden shop overlay plus 1-copper item-price patches.",
        },
    )
    write_json(folder / "build_report.json", {"game_version": context.game_version, "prices": price_meta, "store": store_meta})
    (folder / "README.txt").write_text(
        f"Alden AIO Shop + All Items 1 Copper - Crimson Desert {context.game_version}\n\n"
        f"Offers {store_meta['rebuilt_offer_count']} items and remaps {price_meta['changes']} non-1 prices to one copper.\n"
        "All other current store records are preserved byte-for-byte.\n",
        encoding="utf-8",
        newline="\n",
    )
    package = _package(folder, context.normal_root / "packages")
    return BuildResult(name="alden", status="ok", packages=[package], details={"prices": price_meta, "store": store_meta})


def _rebuild_healthbar_skill(context: BuildContext) -> tuple[bytes, bytes, dict[str, Any]]:
    recipe = context.recipes.healthbar
    data = (context.binary_root / "skill.pabgb").read_bytes()
    header = (context.binary_root / "skill.pabgh").read_bytes()
    records = parse_u32_header(data, header)
    target_key = int(recipe["skill_target_key"])
    source_key = int(recipe["skill_source_key"])
    target = next((record for record in records if record.key == target_key), None)
    source = next((record for record in records if record.key == source_key), None)
    if target is None or source is None:
        raise RuntimeError("Healthbar source or target skill is missing")
    if target.name != recipe["skill_target_name"] or source.name != recipe["skill_source_name"]:
        raise RuntimeError(f"Healthbar skill names changed: {target.name} / {source.name}")

    source_bytes = data[source.start:source.end]
    source_name_length = struct.unpack_from("<I", source_bytes, 4)[0]
    source_body = source_bytes[8 + source_name_length:]
    target_name = recipe["skill_target_name"].encode("utf-8")
    replacement = bytearray(struct.pack("<II", target_key, len(target_name)) + target_name + source_body)
    source_key_bytes = struct.pack("<I", source_key)
    target_key_bytes = struct.pack("<I", target_key)
    self_references = find_all(bytes(replacement), source_key_bytes)
    if len(self_references) != 1:
        raise RuntimeError(f"Healthbar skill self-reference changed: {self_references}")
    replacement[self_references[0]:self_references[0] + 4] = target_key_bytes
    replacement_bytes = bytes(replacement)

    parts: list[bytes] = []
    offsets: list[int] = []
    cursor = 0
    original_records = {record.key: data[record.start:record.end] for record in records}
    for record in records:
        offsets.append(cursor)
        part = replacement_bytes if record.key == target_key else original_records[record.key]
        parts.append(part)
        cursor += len(part)
    rebuilt_data = b"".join(parts)
    rebuilt_header = build_u32_header(records, offsets)
    parsed = parse_u32_header(rebuilt_data, rebuilt_header)
    rebuilt_target = next(record for record in parsed if record.key == target_key)
    if rebuilt_data[rebuilt_target.start:rebuilt_target.end] != replacement_bytes:
        raise RuntimeError("Healthbar rebuilt target skill is invalid")
    for record in parsed:
        if record.key != target_key and rebuilt_data[record.start:record.end] != original_records[record.key]:
            raise RuntimeError(f"Healthbar changed non-target skill {record.key}")

    return rebuilt_data, rebuilt_header, {
        "record_count": len(records),
        "old_target_size": target.size,
        "new_target_size": rebuilt_target.size,
        "offset_delta": rebuilt_target.size - target.size,
        "self_reference_offset": self_references[0],
    }


def _healthbar_character_patches(context: BuildContext) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    data = (context.binary_root / "characterinfo.pabgb").read_bytes()
    records = parse_u32_header(data, (context.binary_root / "characterinfo.pabgh").read_bytes())
    by_key = {record.key: record for record in records}
    patches: list[dict[str, Any]] = []
    meta: list[dict[str, Any]] = []
    for character in context.recipes.healthbar["characters"]:
        key = int(character["key"])
        record = by_key.get(key)
        if record is None or record.name != character["name"]:
            raise RuntimeError(f"Healthbar character changed or missing: {key}")
        payload = data[record.start:record.end]
        locator = ContextLocator.from_recipe(character)
        relative = locate_context(payload, locator)
        current = payload[relative:relative + 4]
        patched = bytes.fromhex(character["patched"])
        if current == patched:
            original = current
        else:
            original = current
        patches.append(
            {
                "description": f"Enable Equip_Passive_ShowHPUI for {record.name} ({record.key}).",
                "file": "gamedata/binary__/client/bin/characterinfo.pabgb",
                "target": "gamedata/binary__/client/bin/characterinfo.pabgb",
                "offset": record.start + relative,
                "label": f"{record.name}: ShowHPUI skill reference, {context.game_version} semantic remap",
                "original": spaced_hex(original),
                "patched": spaced_hex(patched),
            }
        )
        meta.append({"key": key, "name": record.name, "relative_offset": relative, "absolute_offset": record.start + relative})
    ensure_unique((int(patch["offset"]) for patch in patches), "Healthbar character offsets")
    return patches, meta


def _healthbar_ui(context: BuildContext, variant: str) -> tuple[bytes, bytes, dict[str, Any]]:
    html = (context.ui_root / "subtitletagview.html").read_bytes().decode("utf-8-sig")
    css = (context.ui_root / "subtitletagview.css").read_bytes().decode("utf-8-sig")
    operations: dict[str, Any] = {}

    html, count = re.subn(r'\s+ShowHPBuffTag="CheckShowHP"', "", html, count=1)
    operations["removed_ShowHPBuffTag"] = count
    html, hidden_root_count = re.subn(
        r'css="no-pickable fit-all cpp-head-up !cpp-none"',
        'css="no-pickable fit-all cpp-head-up"',
        html,
        count=1,
    )
    operations["removed_root_hidden_class"] = hidden_root_count
    html, gauge_count = re.subn(
        r'css="headup-hp-stat-wrap !cpp-none" component="CharacterStat\.StatusProgressingGaugebar" statName="Hp" selector-show-layer-list="#HPGauge"',
        'css="headup-hp-stat-wrap" component="CharacterStat.StatusProgressingGaugebar" statName="Hp"',
        html,
        count=1,
    )
    operations["removed_gauge_gate"] = gauge_count
    if count != 1 or hidden_root_count != 1 or gauge_count != 1:
        raise RuntimeError(f"Healthbar HTML structure changed: {operations}")

    lines = css.splitlines()
    starts = [index for index, line in enumerate(lines) if line.startswith(".headup-hp-stat-wrap ")]
    if len(starts) != 1:
        raise RuntimeError(f"Healthbar CSS block start changed: {starts}")
    start = starts[0]
    end = start + 8
    if end >= len(lines) or not lines[end].startswith(".headup-hp-stat-gauge-tip "):
        raise RuntimeError("Healthbar CSS block shape changed")

    compact = [
        ".headup-hp-stat-wrap { position: absolute; top: -35px; width: 120px; display: flex; justify-content: flex-start; align-items: center; height: auto; margin-top: 3px; margin-bottom: 3px; translate-x: -50%; transform: scaleX(0.7) scaleY(0.7); opacity: 1; }",
        ".headup-hp-stat-gauge-dimmed { background-color: #e0e0e0; width: 150px; height: 11px; position: absolute; top: -3px; left: -15px; box-sizing: content-box; background-image: textureid(cd_hud_nomal_gauge_dimmed_bg); background-blend-mode: multiply; opacity: 0; }",
        ".headup-hp-stat-gauge-wrap { width: 260px; height: 5px; }",
        ".headup-hp-stat-gauge-bar-back { position: absolute; top: -13px; left: 0; width: 100%; height: 31px; box-sizing: content-box; background-image: textureid(cd_hud_gauge_glow_white_body); background-blend-mode: multiply; opacity: 0; }",
        ".headup-hp-stat-gauge-bar-back.cpp-increase { background-color: #54c8a5; }",
        ".headup-hp-stat-gauge-bar-back.cpp-decrease { background-color: #ffffff; }",
        ".headup-hp-stat-gauge-bar-bg { position: absolute; top: 0; left: 0; width: 100%; height: 5px; box-sizing: content-box; background-image: textureid(cd_common_gauge_00); background-color: #5d5d5d; background-blend-mode: multiply; opacity: 0; }",
        ".headup-hp-stat-gauge-bar-front { position: absolute; top: 0; left: 0; width: 120px; height: 5px; box-sizing: content-box; background-image: textureid(cd_common_gauge_00); background-color: #d35555; background-blend-mode: multiply; }",
        ".headup-hp-stat-gauge-tip { position: absolute; width: 13px; height: 9px; top: 50%; background-image: textureid(hud_nametag_bg_boss_edge_00); translate-x: -50%; background-color: #c86161; background-blend-mode: overlay; translate-y: -50%; left: 50%; opacity: 0; }",
    ]
    classic = [
        ".headup-hp-stat-wrap { position: absolute; top: -35px; width: 120px; display: flex; justify-content: flex-start; align-items: center; height: auto; margin-top: 3px; margin-bottom: 3px; translate-x: -50%; transform: scaleX(1.0) scaleY(1.0); opacity: 1; }",
        ".headup-hp-stat-gauge-dimmed { background-color: #e0e0e0; width: 150px; height: 17px; position: absolute; top: -3px; left: -15px; box-sizing: content-box; background-image: textureid(cd_hud_nomal_gauge_dimmed_bg); background-blend-mode: multiply; opacity: 0.99; }",
        ".headup-hp-stat-gauge-wrap { width: 120px; height: 11px; }",
        ".headup-hp-stat-gauge-bar-back { position: absolute; top: -13px; left: 0; width: 100%; height: 31px; box-sizing: content-box; background-image: textureid(cd_hud_gauge_glow_white_body); background-blend-mode: multiply; opacity: 0; }",
        ".headup-hp-stat-gauge-bar-back.cpp-increase { background-color: #54c8a5; }",
        ".headup-hp-stat-gauge-bar-back.cpp-decrease { background-color: #ffffff; }",
        ".headup-hp-stat-gauge-bar-bg { position: absolute; top: 0; left: 0; width: 100%; height: 11px; box-sizing: content-box; background-image: textureid(cd_common_gauge_00); background-color: #5d5d5d; background-blend-mode: multiply; opacity: 1; }",
        ".headup-hp-stat-gauge-bar-front { position: absolute; top: 0; left: 0; width: 120px; height: 11px; box-sizing: content-box; background-image: textureid(cd_common_gauge_00); background-color: #d35555; background-blend-mode: multiply; }",
        ".headup-hp-stat-gauge-tip { position: absolute; width: 13px; height: 9px; top: 50%; background-image: textureid(hud_nametag_bg_boss_edge_00); translate-x: -50%; background-color: #c86161; background-blend-mode: overlay; translate-y: -50%; left: 50%; opacity: 0; }",
    ]
    lines[start:end + 1] = compact if variant == "caites" else classic
    if variant == "caites":
        lines += [
            "",
            "/* Keep active compact enemy HP bars visible. */",
            ".cpp-head-up #HPGauge { display: flex; opacity: 1; }",
            ".cpp-head-up.cpp-hide #HPGauge { display: flex; opacity: 1; animation: none; }",
        ]
    elif variant == "multi":
        lines += [
            "",
            "/* Preserve multiple emitted enemy HP bars. */",
            ".headup-hp-stat-wrap.cpp-show { display: flex; opacity: 1; }",
            ".headup-hp-stat-wrap.cpp-hide { display: flex; opacity: 1; animation: none; }",
            ".cpp-head-up #HPGauge { display: flex; opacity: 1; }",
            ".cpp-head-up.cpp-hide #HPGauge { display: flex; opacity: 1; animation: none; }",
        ]
    elif variant != "classic":
        raise RuntimeError(f"Unknown Healthbar variant: {variant}")

    eol = "\r\n"
    html_bytes = b"\xef\xbb\xbf" + (html.rstrip("\r\n") + eol).replace("\r\n", "\n").replace("\n", eol).encode("utf-8")
    css_bytes = b"\xef\xbb\xbf" + (eol.join(lines) + eol).encode("utf-8")
    return html_bytes, css_bytes, operations


def _healthbar_variant_metadata(game_version: str, variant: str) -> tuple[str, str, str]:
    if variant == "caites":
        return (
            f"Healthbar_always_on_{game_version}",
            "Healthbar always on - Caites multitarget",
            "Compact persistent enemy HP bars with the Caites-style presentation.",
        )
    if variant == "classic":
        return (
            f"Healthbar_always_on_classic_vanilla_single_target_{game_version}",
            "Healthbar always on - classic vanilla single target",
            "Classic vanilla-style single-target enemy HP bar.",
        )
    if variant == "multi":
        return (
            f"Healthbar_always_on_vanilla_multitarget_{game_version}",
            "Healthbar always on - vanilla multitarget",
            "Vanilla-style persistent multi-target enemy HP bars.",
        )
    raise RuntimeError(f"Unknown Healthbar variant: {variant}")


def build_healthbars(context: BuildContext) -> BuildResult:
    skill_data, skill_header, skill_meta = _rebuild_healthbar_skill(context)
    character_patches, character_meta = _healthbar_character_patches(context)
    normal_packages: list[Path] = []
    dmm_packages: list[Path] = []
    variants_meta: dict[str, Any] = {}

    for variant in context.recipes.healthbar["variants"]:
        dirname, title, description = _healthbar_variant_metadata(context.game_version, variant)
        html, css, ui_meta = _healthbar_ui(context, variant)

        normal_folder = context.normal_root / dirname
        normal_skill = normal_folder / "files/0008/gamedata/binary__/client/bin"
        normal_ui = normal_folder / "files/0012/ui/xml/gamemain/play"
        normal_skill.mkdir(parents=True, exist_ok=True)
        normal_ui.mkdir(parents=True, exist_ok=True)
        (normal_skill / "skill.pabgb").write_bytes(skill_data)
        (normal_skill / "skill.pabgh").write_bytes(skill_header)
        (normal_ui / "subtitletagview.html").write_bytes(html)
        (normal_ui / "subtitletagview.css").write_bytes(css)
        full_title = f"{title} - {context.game_version}"
        full_description = f"{description} Rebuilt for Crimson Desert {context.game_version}."
        write_json(
            normal_folder / "Healthbar_always_on.json",
            {
                "name": full_title,
                "version": context.game_version,
                "author": "Blablup / UI persistence by Caites",
                "description": full_description,
                "allow_partial_apply": False,
                "patches": character_patches,
            },
        )
        write_json(
            normal_folder / "manifest.json",
            {
                "format": "crimson_browser_mod_v1",
                "id": f"healthbar_{variant}_{context.game_version.replace('.', '_')}",
                "title": full_title,
                "author": "Blablup / UI persistence by Caites",
                "version": context.game_version,
                "description": full_description,
                "enabled": True,
                "priority": 1,
                "files_dir": "files",
            },
        )
        write_json(
            normal_folder / "modinfo.json",
            {
                "name": full_title,
                "version": context.game_version,
                "author": "Blablup / UI persistence by Caites",
                "description": full_description,
                "conflict_mode": "normal",
            },
        )
        write_json(
            normal_folder / "build_report.json",
            {
                "game_version": context.game_version,
                "variant": variant,
                "skill": skill_meta,
                "characters": character_meta,
                "ui": ui_meta,
            },
        )
        (normal_folder / "README_Healthbar_always_on.txt").write_text(
            f"{full_title}\n\n{full_description}\n\nInstall exactly one Healthbar variant.\n",
            encoding="utf-8",
            newline="\n",
        )
        normal_packages.append(_package(normal_folder, context.normal_root / "packages"))

        dmm_dirname = f"{dirname}_DMM"
        dmm_folder = context.dmm_root / dmm_dirname
        dmm_skill = dmm_folder / "files/0008/gamedata/binary__/client/bin"
        dmm_ui = dmm_folder / "files/0012/ui/xml/gamemain/play"
        dmm_skill.mkdir(parents=True, exist_ok=True)
        dmm_ui.mkdir(parents=True, exist_ok=True)
        (dmm_skill / "skill.pabgb").write_bytes(skill_data)
        (dmm_skill / "skill.pabgh").write_bytes(skill_header)
        (dmm_ui / "subtitletagview.html").write_bytes(html)
        (dmm_ui / "subtitletagview.css").write_bytes(css)
        write_json(
            dmm_folder / f"{dmm_dirname}.json",
            {
                "name": f"{full_title} (DMM)",
                "version": context.game_version,
                "author": "Blablup / UI persistence by Caites",
                "description": f"{full_description} DMM package.",
                "allow_partial_apply": False,
                "patches": character_patches,
                "dmm_offsetautorelocate_disable": True,
                "dmm_v3upgrade_disable": True,
            },
        )
        (dmm_folder / "README.txt").write_text(
            f"{full_title} (DMM)\n\n"
            "In DMM: Revert to Vanilla, remove old Healthbar packages, import this ZIP, enable only one variant, then mount.\n",
            encoding="utf-8",
            newline="\n",
        )
        dmm_packages.append(_package(dmm_folder, context.dmm_root / "packages"))
        variants_meta[variant] = {"html_bytes": len(html), "css_bytes": len(css)}

    return BuildResult(
        name="healthbars",
        status="ok",
        packages=normal_packages + dmm_packages,
        details={"skill": skill_meta, "characters": character_meta, "variants": variants_meta},
    )


def validate_patch_package(package: Path, game_root: Path) -> dict[str, Any]:
    """Validate all JSON patch originals embedded in a generated package."""
    mapping = {
        "gamedata/characterinfo.pabgb": game_root / "0008/gamedata/binary__/client/bin/characterinfo.pabgb",
        "gamedata/iteminfo.pabgb": game_root / "0008/gamedata/binary__/client/bin/iteminfo.pabgb",
        "gamedata/binary__/client/bin/characterinfo.pabgb": game_root / "0008/gamedata/binary__/client/bin/characterinfo.pabgb",
    }
    checked = 0
    with zipfile.ZipFile(package, "r") as archive:
        bad = archive.testzip()
        if bad:
            raise RuntimeError(f"Corrupt package {package}: {bad}")
        json_members = [name for name in archive.namelist() if name.lower().endswith(".json")]
        for member in json_members:
            import json

            payload = json.loads(archive.read(member).decode("utf-8-sig"))
            for patch_set in payload.get("patches", []):
                if "changes" in patch_set:
                    path = mapping[patch_set["game_file"]]
                    data = path.read_bytes()
                    for change in patch_set["changes"]:
                        offset = int(change["offset"])
                        original = bytes.fromhex(change["original"])
                        if data[offset:offset + len(original)] != original:
                            raise RuntimeError(f"Patch original mismatch in {package.name} at {offset}")
                        checked += 1
                elif "offset" in patch_set:
                    target = patch_set.get("target") or patch_set["file"]
                    path = mapping[target]
                    data = path.read_bytes()
                    offset = int(patch_set["offset"])
                    original = bytes.fromhex(patch_set["original"])
                    if data[offset:offset + len(original)] != original:
                        raise RuntimeError(f"Patch original mismatch in {package.name} at {offset}")
                    checked += 1
    return {"package": package.name, "patches_checked": checked, "sha256": sha256_file(package)}

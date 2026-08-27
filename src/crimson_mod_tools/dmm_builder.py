from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import struct
import tempfile
import zipfile
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Sequence

GAME_VERSION = "2.00.00"
DMM_VERSION = "1.9.3"
AUTHOR = "Blablup"
HEALTHBAR_AUTHOR = "Blablup / UI persistence by Caites"

BINARY_REL = Path("0008/gamedata/binary__/client/bin")
UI_REL = Path("0012/ui/xml/gamemain/play")
TARGET_CHARACTER = "gamedata/binary__/client/bin/characterinfo.pabgb"

REQUIRED_FILES = (
    BINARY_REL / "characterinfo.pabgb",
    BINARY_REL / "characterinfo.pabgh",
    BINARY_REL / "iteminfo.pabgb",
    BINARY_REL / "iteminfo.pabgh",
    BINARY_REL / "skill.pabgb",
    BINARY_REL / "skill.pabgh",
    BINARY_REL / "storeinfo.pabgb",
    BINARY_REL / "storeinfo.pabgh",
    BINARY_REL / "buffinfo.pabgb",
    BINARY_REL / "buffinfo.pabgh",
    UI_REL / "subtitletagview.css",
    UI_REL / "subtitletagview.html",
)


@dataclass(frozen=True)
class Record:
    key: int
    start: int
    end: int
    name: str

    @property
    def size(self) -> int:
        return self.end - self.start


@dataclass(frozen=True)
class StoreEntry:
    item_id: int
    position: int
    payload: bytes

    @property
    def size(self) -> int:
        return len(self.payload)


def write_json(path: Path, payload: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text.rstrip() + "\n", encoding="utf-8", newline="\n")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def ensure_game_root(candidate: Path) -> Path:
    candidate = candidate.resolve()
    if all((candidate / relative).is_file() for relative in REQUIRED_FILES):
        return candidate
    roots: list[Path] = []
    for hit in candidate.rglob("characterinfo.pabgb"):
        possible = hit
        for _ in BINARY_REL.parts:
            possible = possible.parent
        if all((possible / relative).is_file() for relative in REQUIRED_FILES):
            roots.append(possible)
    roots = sorted(set(roots))
    if len(roots) != 1:
        raise RuntimeError(f"Could not identify exactly one game root below {candidate}. Candidates: {roots}")
    return roots[0]


def parse_u32_records(data: bytes, header: bytes) -> list[Record]:
    if len(header) < 2:
        raise RuntimeError("Table header is truncated")
    count = struct.unpack_from("<H", header, 0)[0]
    if len(header) != 2 + count * 8:
        raise RuntimeError("Unexpected u32 table header size")
    index = [struct.unpack_from("<II", header, 2 + i * 8) for i in range(count)]
    records: list[Record] = []
    for i, (key, start) in enumerate(index):
        end = index[i + 1][1] if i + 1 < count else len(data)
        if not (0 <= start < end <= len(data)):
            raise RuntimeError(f"Invalid record range for key {key}: {start}:{end}")
        embedded_key, name_len = struct.unpack_from("<II", data, start)
        if embedded_key != key or start + 8 + name_len > end:
            raise RuntimeError(f"Record prefix mismatch for key {key}")
        name = data[start + 8 : start + 8 + name_len].decode("utf-8")
        records.append(Record(key, start, end, name))
    return records


def build_u32_header(records: Sequence[tuple[int, int]]) -> bytes:
    if len(records) > 0xFFFF:
        raise RuntimeError("Too many records")
    out = bytearray(struct.pack("<H", len(records)))
    for key, offset in records:
        out += struct.pack("<II", key, offset)
    return bytes(out)


def parse_store_records(data: bytes, header: bytes) -> list[Record]:
    if len(header) < 2:
        raise RuntimeError("Store header is truncated")
    count = struct.unpack_from("<H", header, 0)[0]
    if len(header) != 2 + count * 6:
        raise RuntimeError("Unexpected StoreInfo header size")
    index = [struct.unpack_from("<HI", header, 2 + i * 6) for i in range(count)]
    records: list[Record] = []
    for i, (key, start) in enumerate(index):
        end = index[i + 1][1] if i + 1 < count else len(data)
        embedded_key = struct.unpack_from("<H", data, start)[0]
        name_len = struct.unpack_from("<I", data, start + 2)[0]
        if embedded_key != key or start + 6 + name_len > end:
            raise RuntimeError(f"Store record prefix mismatch for key {key}")
        name = data[start + 6 : start + 6 + name_len].decode("utf-8")
        records.append(Record(key, start, end, name))
    return records


def build_store_header(records: Sequence[tuple[int, int]]) -> bytes:
    if len(records) > 0xFFFF:
        raise RuntimeError("Too many store records")
    out = bytearray(struct.pack("<H", len(records)))
    for key, offset in records:
        out += struct.pack("<HI", key, offset)
    return bytes(out)


def find_all(payload: bytes, needle: bytes) -> list[int]:
    out: list[int] = []
    cursor = 0
    while True:
        position = payload.find(needle, cursor)
        if position < 0:
            return out
        out.append(position)
        cursor = position + 1


def compact_hex(data: bytes) -> str:
    return data.hex().upper()


def direct_patch(target: str, offset: int, original: bytes, patched: bytes, description: str) -> dict[str, Any]:
    if not original or len(original) != len(patched):
        raise RuntimeError(f"Invalid patch length: {description}")
    return {
        "description": description,
        "file": target,
        "target": target,
        "offset": offset,
        "original": compact_hex(original),
        "patched": compact_hex(patched),
    }


def zip_folder(folder: Path, destination: Path) -> Path:
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        destination.unlink()
    with zipfile.ZipFile(destination, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in sorted(folder.rglob("*")):
            if path.is_file():
                archive.write(path, Path(folder.name) / path.relative_to(folder))
    return destination


def build_legacy_manifest(folder: Path, slug: str, description: str, author: str, patches: list[dict[str, Any]]) -> None:
    write_json(
        folder / f"{slug}.json",
        {
            "name": slug,
            "version": GAME_VERSION,
            "author": author,
            "description": description,
            "patches": patches,
            "dmm_offsetautorelocate_disable": True,
            "dmm_v3upgrade_disable": True,
        },
    )


def build_v3_manifest(folder: Path, slug: str, description: str, targets: list[dict[str, Any]]) -> None:
    write_json(
        folder / f"{slug}.json",
        {
            "modinfo": {
                "title": slug,
                "version": GAME_VERSION,
                "author": AUTHOR,
                "description": description,
            },
            "format": 3,
            "format_minor": 1,
            "targets": targets,
        },
    )


def locate_mount_blocks(payload: bytes) -> list[int]:
    blocks: list[int] = []
    for relative in range(0, len(payload) - 15):
        values = struct.unpack_from("<IIII", payload, relative)
        if all(1 <= value <= 5 for value in values):
            if blocks and relative < blocks[-1] + 16:
                continue
            blocks.append(relative)
    return blocks


def build_mount_packages(repo_root: Path, game_root: Path, build_root: Path, packages_root: Path) -> list[Path]:
    data = (game_root / BINARY_REL / "characterinfo.pabgb").read_bytes()
    records = parse_u32_records(data, (game_root / BINARY_REL / "characterinfo.pabgh").read_bytes())
    by_name: dict[str, list[Record]] = defaultdict(list)
    for record in records:
        by_name[record.name].append(record)
    target_file = repo_root / "recipes/2.00.00/mount_targets.txt"
    targets = [line.strip() for line in target_file.read_text(encoding="utf-8").splitlines() if line.strip()]
    if len(targets) != 380 or len(set(targets)) != 380:
        raise RuntimeError("Mount recipe must contain exactly 380 unique records")
    if any(len(by_name[name]) != 1 for name in targets):
        raise RuntimeError("One or more mount records are missing or duplicated")

    speed_patches: list[dict[str, Any]] = []
    all_patches: list[dict[str, Any]] = []
    fields = ("movement speed", "acceleration", "turning", "jump")
    block_count = 0
    for name in targets:
        record = by_name[name][0]
        payload = data[record.start : record.end]
        blocks = locate_mount_blocks(payload)
        block_count += len(blocks)
        for number, relative in enumerate(blocks, start=1):
            values = struct.unpack_from("<IIII", payload, relative)
            if values[0] != 5:
                speed_patches.append(
                    direct_patch(
                        TARGET_CHARACTER,
                        record.start + relative,
                        struct.pack("<I", values[0]),
                        struct.pack("<I", 5),
                        f"{name}, stat block {number}: movement speed {values[0]} -> 5.",
                    )
                )
            for field_index, value in enumerate(values):
                if value == 5:
                    continue
                all_patches.append(
                    direct_patch(
                        TARGET_CHARACTER,
                        record.start + relative + field_index * 4,
                        struct.pack("<I", value),
                        struct.pack("<I", 5),
                        f"{name}, stat block {number}: {fields[field_index]} {value} -> 5.",
                    )
                )
    if (block_count, len(speed_patches), len(all_patches)) != (1250, 1248, 4990):
        raise RuntimeError(
            f"Mount baseline changed: blocks={block_count}, speed={len(speed_patches)}, all={len(all_patches)}"
        )

    variants = (
        (
            "All_Mounts_LvL_5_Speed_2.00.00",
            "Sets only the movement-speed rating in all compatible mount stat blocks to level 5 for Crimson Desert 2.00.00.",
            speed_patches,
        ),
        (
            "All_Mounts_LvL_5_All_Stats_2.00.00",
            "Sets movement speed, acceleration, turning and jump in all compatible mount stat blocks to level 5 for Crimson Desert 2.00.00.",
            all_patches,
        ),
    )
    archives: list[Path] = []
    for slug, description, patches in variants:
        folder = build_root / slug
        folder.mkdir(parents=True, exist_ok=True)
        build_legacy_manifest(folder, slug, description, AUTHOR, patches)
        write_text(
            folder / "README.txt",
            f"{slug}\n\nBuilt for Crimson Desert {GAME_VERSION} and DMM.\n"
            "Import this ZIP directly in DMM. Do not enable both mount variants at the same time.",
        )
        archives.append(zip_folder(folder, packages_root / f"{slug}.zip"))
    return archives


def build_steelheart_package(game_root: Path, build_root: Path, packages_root: Path) -> Path:
    item_data = (game_root / BINARY_REL / "iteminfo.pabgb").read_bytes()
    item_records = parse_u32_records(item_data, (game_root / BINARY_REL / "iteminfo.pabgh").read_bytes())
    item = next((record for record in item_records if record.key == 1000594), None)
    if item is None or item.name != "HorseShoe_HorseArmor_Shoe_III":
        raise RuntimeError("Steelheart item record changed")
    payload = item_data[item.start : item.end]
    if find_all(payload, struct.pack("<II", 1000046, 4)) != [370]:
        raise RuntimeError("Steelheart vanilla EquipBuff layout changed")

    buff_data = (game_root / BINARY_REL / "buffinfo.pabgb").read_bytes()
    buff_records = parse_u32_records(buff_data, (game_root / BINARY_REL / "buffinfo.pabgh").read_bytes())
    buff_names = {record.key: record.name for record in buff_records}
    if buff_names.get(1000046) != "BuffLevel_StaminaRegen":
        raise RuntimeError("Steelheart stamina-regeneration buff changed")
    if buff_names.get(1000213) != "BuffLevel_HorseArmor_Shoe_StaminaRegen":
        raise RuntimeError("HorseArmor stamina-regeneration buff baseline changed")

    slug = "Steelheart_Horseshoes_20_Stamina_Regen_2.00.00"
    folder = build_root / slug
    folder.mkdir(parents=True, exist_ok=True)
    build_v3_manifest(
        folder,
        slug,
        "Sets the Steelheart Horseshoes stamina-regeneration equip-buff level from 4 to 20. Only item 1000594 is targeted.",
        [
            {
                "file": "iteminfo.pabgb",
                "intents": [
                    {
                        "entry": "HorseShoe_HorseArmor_Shoe_III",
                        "key": 1000594,
                        "field": "enchant_data_list[0].equip_buffs[0].level",
                        "op": "set",
                        "new": 20,
                    }
                ],
            }
        ],
    )
    write_text(
        folder / "README.txt",
        f"{slug}\n\nBuilt for Crimson Desert {GAME_VERSION} and DMM.\n"
        "Only HorseShoe_HorseArmor_Shoe_III (1000594) is targeted; stamina regeneration level is set from 4 to 20.",
    )
    return zip_folder(folder, packages_root / f"{slug}.zip")


def build_healthbar_character_patches(game_root: Path) -> list[dict[str, Any]]:
    data = (game_root / BINARY_REL / "characterinfo.pabgb").read_bytes()
    records = parse_u32_records(data, (game_root / BINARY_REL / "characterinfo.pabgh").read_bytes())
    by_key = {record.key: record for record in records}
    patches: list[dict[str, Any]] = []
    for key, expected_name in ((1, "Kliff"), (1002113, "Kliff_AI"), (1004085, "Yann")):
        record = by_key.get(key)
        if record is None or record.name != expected_name:
            raise RuntimeError(f"Healthbar range character changed: {key}/{expected_name}")
        payload = data[record.start : record.end]
        positions = [
            position
            for position in find_all(payload, bytes.fromhex("FA000000"))
            if position >= 8 and payload[position - 8 : position] == bytes.fromhex("FBB0010001000000")
        ]
        if len(positions) != 1:
            raise RuntimeError(f"Healthbar range helper is not unique for {expected_name}: {positions}")
        relative = positions[0]
        patches.append(
            direct_patch(
                TARGET_CHARACTER,
                record.start + relative,
                bytes.fromhex("FA000000"),
                bytes.fromhex("7A630100"),
                f"{expected_name}: redirect HP range helper to Equip_Passive_ShowHPUI.",
            )
        )
    return patches


def build_healthbar_skill_overlay(game_root: Path) -> tuple[bytes, bytes]:
    data = (game_root / BINARY_REL / "skill.pabgb").read_bytes()
    header = (game_root / BINARY_REL / "skill.pabgh").read_bytes()
    records = parse_u32_records(data, header)
    by_key = {record.key: record for record in records}
    target = by_key.get(1201)
    source = by_key.get(91002)
    if target is None or target.name != "Passive_Player_BasicSkill":
        raise RuntimeError("Healthbar target skill 1201 changed")
    if source is None or source.name != "Equip_Passive_ShowHPUI":
        raise RuntimeError("Healthbar source skill 91002 changed")

    target_record = data[target.start : target.end]
    source_record = data[source.start : source.end]
    target_name_len = struct.unpack_from("<I", target_record, 4)[0]
    source_name_len = struct.unpack_from("<I", source_record, 4)[0]
    target_prefix = target_record[: 8 + target_name_len]
    source_payload = source_record[8 + source_name_len :]
    replacement = bytearray(target_prefix + source_payload)

    source_key = struct.pack("<I", 91002)
    target_key = struct.pack("<I", 1201)
    positions = find_all(replacement, source_key)
    if positions != [182]:
        raise RuntimeError(f"Healthbar cloned skill self-reference changed: {positions}")
    replacement[positions[0] : positions[0] + 4] = target_key
    if len(replacement) != 408:
        raise RuntimeError(f"Healthbar cloned skill size changed: {len(replacement)}")

    parts: list[bytes] = []
    index: list[tuple[int, int]] = []
    cursor = 0
    for record in records:
        payload = bytes(replacement) if record.key == 1201 else data[record.start : record.end]
        index.append((record.key, cursor))
        parts.append(payload)
        cursor += len(payload)
    rebuilt_data = b"".join(parts)
    rebuilt_header = build_u32_header(index)
    rebuilt_records = parse_u32_records(rebuilt_data, rebuilt_header)
    if len(rebuilt_records) != len(records) or [r.key for r in rebuilt_records] != [r.key for r in records]:
        raise RuntimeError("Healthbar rebuilt skill table failed validation")
    changed = [
        old.key
        for old, new in zip(records, rebuilt_records)
        if data[old.start : old.end] != rebuilt_data[new.start : new.end]
    ]
    if changed != [1201]:
        raise RuntimeError(f"Healthbar skill overlay changed unexpected records: {changed}")
    return rebuilt_data, rebuilt_header


def healthbar_ui(game_root: Path, variant: str) -> tuple[bytes, bytes]:
    html = (game_root / UI_REL / "subtitletagview.html").read_bytes().decode("utf-8-sig")
    css = (game_root / UI_REL / "subtitletagview.css").read_bytes().decode("utf-8-sig")
    html, a = re.subn(r'\s+ShowHPBuffTag="CheckShowHP"', "", html, count=1)
    html, b = re.subn(
        r'css="no-pickable fit-all cpp-head-up !cpp-none"',
        'css="no-pickable fit-all cpp-head-up"',
        html,
        count=1,
    )
    html, c = re.subn(
        r'css="headup-hp-stat-wrap !cpp-none" component="CharacterStat\.StatusProgressingGaugebar" statName="Hp" selector-show-layer-list="#HPGauge"',
        'css="headup-hp-stat-wrap" component="CharacterStat.StatusProgressingGaugebar" statName="Hp"',
        html,
        count=1,
    )
    if (a, b, c) != (1, 1, 1):
        raise RuntimeError(f"Healthbar HTML schema changed: {(a, b, c)}")

    lines = css.replace("\r\n", "\n").replace("\r", "\n").split("\n")
    selectors = [
        ".headup-hp-stat-wrap ",
        ".headup-hp-stat-gauge-dimmed ",
        ".headup-hp-stat-gauge-wrap ",
        ".headup-hp-stat-gauge-bar-back ",
        ".headup-hp-stat-gauge-bar-back.cpp-increase ",
        ".headup-hp-stat-gauge-bar-back.cpp-decrease ",
        ".headup-hp-stat-gauge-bar-bg ",
        ".headup-hp-stat-gauge-bar-front ",
        ".headup-hp-stat-gauge-tip ",
    ]
    positions: list[int] = []
    for selector in selectors:
        matches = [i for i, line in enumerate(lines) if line.startswith(selector)]
        if len(matches) != 1:
            raise RuntimeError(f"Healthbar CSS selector changed: {selector} -> {matches}")
        positions.append(matches[0])
    if positions != list(range(positions[0], positions[0] + 9)):
        raise RuntimeError(f"Healthbar CSS block is not contiguous: {positions}")

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
    start = positions[0]
    lines[start : start + 9] = compact if variant in ("caites", "multi") else classic
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
        raise RuntimeError(f"Unknown healthbar variant: {variant}")

    eol = "\r\n"
    html_bytes = b"\xef\xbb\xbf" + (html.rstrip("\r\n") + eol).replace("\r\n", "\n").replace("\n", eol).encode("utf-8")
    css_bytes = b"\xef\xbb\xbf" + (eol.join(lines).rstrip("\r\n") + eol).encode("utf-8")
    return html_bytes, css_bytes


def build_healthbar_packages(game_root: Path, build_root: Path, packages_root: Path) -> list[Path]:
    patches = build_healthbar_character_patches(game_root)
    skill_data, skill_header = build_healthbar_skill_overlay(game_root)
    variants = (
        (
            "caites",
            "Healthbar_always_on_2.00.00",
            "Persistent enemy HP bars with the Caites-style multitarget presentation for Crimson Desert 2.00.00.",
        ),
        (
            "classic",
            "Healthbar_always_on_classic_vanilla_single_target_2.00.00",
            "Persistent classic vanilla-style single-target enemy HP bar for Crimson Desert 2.00.00.",
        ),
        (
            "multi",
            "Healthbar_always_on_vanilla_multitarget_2.00.00",
            "Persistent vanilla-style multitarget enemy HP bars for Crimson Desert 2.00.00.",
        ),
    )
    archives: list[Path] = []
    for variant, slug, description in variants:
        folder = build_root / slug
        binary = folder / "files" / BINARY_REL
        ui = folder / "files" / UI_REL
        binary.mkdir(parents=True, exist_ok=True)
        ui.mkdir(parents=True, exist_ok=True)
        (binary / "skill.pabgb").write_bytes(skill_data)
        (binary / "skill.pabgh").write_bytes(skill_header)
        html, css = healthbar_ui(game_root, variant)
        (ui / "subtitletagview.html").write_bytes(html)
        (ui / "subtitletagview.css").write_bytes(css)
        build_legacy_manifest(folder, slug, description, HEALTHBAR_AUTHOR, patches)
        write_text(
            folder / "README.txt",
            f"{slug}\n\nBuilt for Crimson Desert {GAME_VERSION} and DMM.\n"
            "Import this ZIP directly in DMM. Enable only one Healthbar variant at a time.",
        )
        archives.append(zip_folder(folder, packages_root / f"{slug}.zip"))
    return archives


def detect_store_entries(record: Record, payload: bytes) -> list[StoreEntry]:
    starts: list[int] = []
    key_bytes = struct.pack("<H", record.key)
    for position in find_all(payload, key_bytes):
        if position + 106 > len(payload):
            continue
        first = struct.unpack_from("<I", payload, position + 43)[0]
        second = struct.unpack_from("<I", payload, position + 102)[0]
        if first == second and first > 0:
            starts.append(position)
    starts = sorted(set(starts))
    out: list[StoreEntry] = []
    for index, position in enumerate(starts):
        end = starts[index + 1] if index + 1 < len(starts) else len(payload) - 17
        entry = payload[position:end]
        if len(entry) not in (119, 132):
            continue
        item_id = struct.unpack_from("<I", entry, 43)[0]
        if struct.unpack_from("<I", entry, 102)[0] != item_id:
            continue
        out.append(StoreEntry(item_id, position, entry))
    return out


def load_alden_catalog(repo_root: Path) -> list[tuple[int, int, int]]:
    rows: list[tuple[int, int, int]] = []
    for line in (repo_root / "recipes/2.00.00/alden_catalog.tsv").read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        index, item_id, stock = map(int, line.split("\t")[:3])
        rows.append((index, item_id, stock))
    if len(rows) != 379 or [row[0] for row in rows] != list(range(379)):
        raise RuntimeError("Alden catalog must contain exactly 379 ordered rows")
    if len({row[1] for row in rows}) != 379:
        raise RuntimeError("Alden catalog item IDs must be unique")
    if Counter(row[2] for row in rows) != Counter({1: 258, 99: 93, 999: 28}):
        raise RuntimeError("Alden stock distribution changed unexpectedly")
    return rows


def build_alden_package(repo_root: Path, game_root: Path, build_root: Path, packages_root: Path) -> Path:
    catalog = load_alden_catalog(repo_root)
    item_data = (game_root / BINARY_REL / "iteminfo.pabgb").read_bytes()
    item_records = parse_u32_records(item_data, (game_root / BINARY_REL / "iteminfo.pabgh").read_bytes())
    item_by_key = {record.key: record for record in item_records}
    missing = [item_id for _, item_id, _ in catalog if item_id not in item_by_key]
    if missing:
        raise RuntimeError(f"Alden catalog items missing in current ItemInfo: {missing[:20]}")

    store_data = (game_root / BINARY_REL / "storeinfo.pabgb").read_bytes()
    store_header = (game_root / BINARY_REL / "storeinfo.pabgh").read_bytes()
    store_records = parse_store_records(store_data, store_header)
    target = next((record for record in store_records if record.key == 3101), None)
    if target is None or target.name != "Store_Her_General":
        raise RuntimeError("Alden store record changed")
    target_payload = store_data[target.start : target.end]
    entries = detect_store_entries(target, target_payload)
    if len(entries) != 40:
        raise RuntimeError(f"Unexpected vanilla Alden offer count: {len(entries)}")
    if struct.unpack_from("<I", target_payload, 60)[0] != 40 or struct.unpack_from("<I", target_payload, 69)[0] != 40:
        raise RuntimeError("Alden count fields changed")
    if target_payload[-17:].hex() != "00000000020000003d3e00000000000001":
        raise RuntimeError("Alden trailer changed")
    template_entry = next((entry for entry in entries if entry.size == 119 and entry.payload[41] == 0), None)
    if template_entry is None:
        raise RuntimeError("Could not locate unrestricted 119-byte Alden template row")
    template = template_entry.payload

    rows: list[bytes] = []
    intents: list[dict[str, Any]] = []
    for index, item_id, stock in catalog:
        row = bytearray(template)
        struct.pack_into("<I", row, 18, stock)
        struct.pack_into("<I", row, 26, index)
        struct.pack_into("<I", row, 30, index)
        row[41] = 0
        struct.pack_into("<I", row, 43, item_id)
        struct.pack_into("<I", row, 102, item_id)
        rows.append(bytes(row))
        item = item_by_key[item_id]
        intents.append(
            {
                "entry": item.name,
                "key": item_id,
                "field": "price_list[*].price.price",
                "op": "set",
                "new": 1,
            }
        )

    prefix = bytearray(target_payload[:73])
    struct.pack_into("<I", prefix, 60, len(rows))
    struct.pack_into("<I", prefix, 69, len(rows))
    rebuilt_target = bytes(prefix) + b"".join(rows) + target_payload[-17:]

    parts: list[bytes] = []
    index_records: list[tuple[int, int]] = []
    cursor = 0
    for record in store_records:
        payload = rebuilt_target if record.key == 3101 else store_data[record.start : record.end]
        index_records.append((record.key, cursor))
        parts.append(payload)
        cursor += len(payload)
    rebuilt_store = b"".join(parts)
    rebuilt_header = build_store_header(index_records)
    rebuilt_records = parse_store_records(rebuilt_store, rebuilt_header)
    if [record.key for record in rebuilt_records] != [record.key for record in store_records]:
        raise RuntimeError("Alden rebuilt StoreInfo index changed")
    for old, new in zip(store_records, rebuilt_records):
        if old.key == 3101:
            continue
        if store_data[old.start : old.end] != rebuilt_store[new.start : new.end]:
            raise RuntimeError(f"Alden changed non-target store {old.key}/{old.name}")
    rebuilt_target_record = next(record for record in rebuilt_records if record.key == 3101)
    rebuilt_entries = detect_store_entries(
        rebuilt_target_record,
        rebuilt_store[rebuilt_target_record.start : rebuilt_target_record.end],
    )
    if len(rebuilt_entries) != 379:
        raise RuntimeError(f"Alden rebuilt catalog count mismatch: {len(rebuilt_entries)}")
    for expected, entry in zip(catalog, rebuilt_entries):
        index, item_id, stock = expected
        if entry.item_id != item_id or struct.unpack_from("<I", entry.payload, 18)[0] != stock:
            raise RuntimeError(f"Alden catalog round-trip mismatch at {index}")

    slug = "Alden_AIO_Shop_All_Items_1_Copper_2.00.00"
    folder = build_root / slug
    overlay = folder / "files" / BINARY_REL
    overlay.mkdir(parents=True, exist_ok=True)
    (overlay / "storeinfo.pabgb").write_bytes(rebuilt_store)
    (overlay / "storeinfo.pabgh").write_bytes(rebuilt_header)
    build_v3_manifest(
        folder,
        slug,
        "Ports the original 379-item Alden shop catalog to Crimson Desert 2.00.00 with the original item order and stock quantities. All listed purchase prices are set to 1 copper.",
        [{"file": "iteminfo.pabgb", "intents": intents}],
    )
    write_text(
        folder / "README.txt",
        f"{slug}\n\nBuilt for Crimson Desert {GAME_VERSION} and DMM.\n"
        "Contains the exact original 379-item catalog, original order and original stock quantities. All catalog purchase prices are set to 1 Copper.",
    )
    return zip_folder(folder, packages_root / f"{slug}.zip")


def validate_output_packages(packages_root: Path) -> dict[str, Any]:
    expected = {
        "Alden_AIO_Shop_All_Items_1_Copper_2.00.00.zip",
        "All_Mounts_LvL_5_All_Stats_2.00.00.zip",
        "All_Mounts_LvL_5_Speed_2.00.00.zip",
        "Steelheart_Horseshoes_20_Stamina_Regen_2.00.00.zip",
        "Healthbar_always_on_2.00.00.zip",
        "Healthbar_always_on_classic_vanilla_single_target_2.00.00.zip",
        "Healthbar_always_on_vanilla_multitarget_2.00.00.zip",
    }
    actual = {path.name for path in packages_root.glob("*.zip")}
    if actual != expected:
        raise RuntimeError(f"Output package set mismatch: expected={sorted(expected)}, actual={sorted(actual)}")
    details: dict[str, Any] = {}
    for path in sorted(packages_root.glob("*.zip")):
        with zipfile.ZipFile(path, "r") as archive:
            corrupt = archive.testzip()
            if corrupt:
                raise RuntimeError(f"Corrupt ZIP member: {path.name}/{corrupt}")
            stem = path.stem
            if not all(name.startswith(stem + "/") for name in archive.namelist()):
                raise RuntimeError(f"Package root mismatch in {path.name}")
            manifests = [name for name in archive.namelist() if name.lower().endswith(".json")]
            if len(manifests) != 1:
                raise RuntimeError(f"Manifest count mismatch in {path.name}")
            manifest = json.loads(archive.read(manifests[0]).decode("utf-8-sig"))
            if "modinfo" in manifest:
                shown_name = manifest["modinfo"]["title"]
                version = manifest["modinfo"]["version"]
            else:
                shown_name = manifest["name"]
                version = manifest["version"]
            if shown_name != stem or version != GAME_VERSION:
                raise RuntimeError(f"Package metadata mismatch in {path.name}")
            if any(token in shown_name.upper() for token in ("_DMM", "_RC", "_TEST", "_R2", "_R3", "_R4", "_R5")):
                raise RuntimeError(f"Test suffix leaked into final package name: {shown_name}")
            details[path.name] = {"sha256": sha256_file(path), "manifest": manifests[0]}
    return details


def build_all(repo_root: Path, game_root: Path) -> dict[str, Any]:
    game_root = ensure_game_root(game_root)
    dist_root = repo_root / "dist" / GAME_VERSION
    build_root = dist_root / "build"
    packages_root = dist_root / "packages"
    shutil.rmtree(dist_root, ignore_errors=True)
    build_root.mkdir(parents=True, exist_ok=True)
    packages_root.mkdir(parents=True, exist_ok=True)

    build_mount_packages(repo_root, game_root, build_root, packages_root)
    build_steelheart_package(game_root, build_root, packages_root)
    build_healthbar_packages(game_root, build_root, packages_root)
    build_alden_package(repo_root, game_root, build_root, packages_root)
    shutil.rmtree(build_root, ignore_errors=True)

    package_validation = validate_output_packages(packages_root)
    checksum_lines = [f"{info['sha256']}  {name}" for name, info in sorted(package_validation.items())]
    write_text(dist_root / "SHA256SUMS.txt", "\n".join(checksum_lines))
    report = {
        "status": "BUILT_AND_STATICALLY_VALIDATED",
        "game_version": GAME_VERSION,
        "target_manager": f"DMM {DMM_VERSION}+",
        "mode": "DMM-only",
        "packages": package_validation,
        "required_input_files": [str(path).replace("\\", "/") for path in REQUIRED_FILES],
        "known_conflicts": [
            "Use only one of the two mount variants at the same time.",
            "Use only one of the three Healthbar variants at the same time.",
        ],
    }
    write_json(dist_root / "BUILD_REPORT.json", report)
    return report


def extract_zip(zip_path: Path) -> tuple[tempfile.TemporaryDirectory[str], Path]:
    temp = tempfile.TemporaryDirectory(prefix="crimson_desert_build_")
    root = Path(temp.name)
    with zipfile.ZipFile(zip_path, "r") as archive:
        if archive.testzip():
            temp.cleanup()
            raise RuntimeError("Input game ZIP is corrupt")
        archive.extractall(root)
    return temp, ensure_game_root(root)


def main() -> int:
    parser = argparse.ArgumentParser(description="Build the Crimson Desert 2.00.00 DMM-only mod set.")
    parser.add_argument("--repo-root", type=Path, required=True)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--game-root", type=Path)
    group.add_argument("--game-zip", type=Path)
    args = parser.parse_args()
    repo_root = args.repo_root.resolve()
    temp: tempfile.TemporaryDirectory[str] | None = None
    try:
        if args.game_zip:
            temp, game_root = extract_zip(args.game_zip.resolve())
        else:
            game_root = ensure_game_root(args.game_root.resolve())
        report = build_all(repo_root, game_root)
        print(json.dumps({"status": report["status"], "packages": len(report["packages"]), "output": str(repo_root / "dist" / GAME_VERSION / "packages")}, indent=2))
        return 0
    finally:
        if temp is not None:
            temp.cleanup()


if __name__ == "__main__":
    raise SystemExit(main())

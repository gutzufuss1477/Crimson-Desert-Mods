from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class PazEntry:
    path: str
    offset: int
    stored_size: int
    original_size: int
    flags: int


def _u32(value: int) -> int:
    return value & 0xFFFFFFFF


def _rotl(value: int, count: int) -> int:
    value &= 0xFFFFFFFF
    return ((value << count) | (value >> (32 - count))) & 0xFFFFFFFF


def _rotr(value: int, count: int) -> int:
    value &= 0xFFFFFFFF
    return ((value >> count) | (value << (32 - count))) & 0xFFFFFFFF


def pearl_abyss_checksum(data: bytes) -> int:
    length = len(data)
    a = b = c = _u32(length - 0x2145E233)
    offset = 0
    remaining = length
    while remaining > 12:
        a = _u32(a + struct.unpack_from("<I", data, offset)[0])
        b = _u32(b + struct.unpack_from("<I", data, offset + 4)[0])
        c = _u32(c + struct.unpack_from("<I", data, offset + 8)[0])
        a = _u32(a - c); a ^= _rotl(c, 4); c = _u32(c + b)
        b = _u32(b - a); b ^= _rotl(a, 6); a = _u32(a + c)
        c = _u32(c - b); c ^= _rotl(b, 8); b = _u32(b + a)
        a = _u32(a - c); a ^= _rotl(c, 16); c = _u32(c + b)
        b = _u32(b - a); b ^= _rotl(a, 19); a = _u32(a + c)
        c = _u32(c - b); c ^= _rotl(b, 4); b = _u32(b + a)
        offset += 12
        remaining -= 12
    tail = data[offset:offset + remaining]
    if remaining >= 12: c = _u32(c + (tail[11] << 24))
    if remaining >= 11: c = _u32(c + (tail[10] << 16))
    if remaining >= 10: c = _u32(c + (tail[9] << 8))
    if remaining >= 9: c = _u32(c + tail[8])
    if remaining >= 8: b = _u32(b + (tail[7] << 24))
    if remaining >= 7: b = _u32(b + (tail[6] << 16))
    if remaining >= 6: b = _u32(b + (tail[5] << 8))
    if remaining >= 5: b = _u32(b + tail[4])
    if remaining >= 4: a = _u32(a + (tail[3] << 24))
    if remaining >= 3: a = _u32(a + (tail[2] << 16))
    if remaining >= 2: a = _u32(a + (tail[1] << 8))
    if remaining >= 1: a = _u32(a + tail[0])
    elif remaining == 0:
        return c
    v82 = _u32((b ^ c) - _rotl(b, 14))
    v83 = _u32((a ^ v82) - _rotl(v82, 11))
    v84 = _u32((v83 ^ b) - _rotr(v83, 7))
    v85 = _u32((v84 ^ v82) - _rotl(v84, 16))
    temp = _u32((v83 ^ v85) - _rotl(v85, 4))
    v87 = _u32((temp ^ v84) - _rotl(temp, 14))
    return _u32((v87 ^ v85) - _rotr(v87, 8))


def parse_pamt(pamt: bytes) -> list[PazEntry]:
    offset = 4
    paz_count = struct.unpack_from("<I", pamt, offset)[0]
    offset += 12
    for index in range(paz_count):
        offset += 8
        if index < paz_count - 1:
            offset += 4
    folder_size = struct.unpack_from("<I", pamt, offset)[0]
    offset += 4
    folder_end = offset + folder_size
    folder_prefix = ""
    while offset < folder_end:
        parent = struct.unpack_from("<I", pamt, offset)[0]
        name_length = pamt[offset + 4]
        name = pamt[offset + 5:offset + 5 + name_length].decode("utf-8", errors="replace")
        if parent == 0xFFFFFFFF:
            folder_prefix = name
        offset += 5 + name_length
    node_size = struct.unpack_from("<I", pamt, offset)[0]
    offset += 4
    node_start = offset
    nodes: dict[int, tuple[int, str]] = {}
    while offset < node_start + node_size:
        relative = offset - node_start
        parent = struct.unpack_from("<I", pamt, offset)[0]
        name_length = pamt[offset + 4]
        name = pamt[offset + 5:offset + 5 + name_length].decode("utf-8", errors="replace")
        nodes[relative] = (parent, name)
        offset += 5 + name_length

    def build_path(node_reference: int) -> str:
        parts: list[str] = []
        current = node_reference
        while current != 0xFFFFFFFF:
            if current not in nodes:
                raise RuntimeError(f"Broken PAMT node reference: {current}")
            parent, name = nodes[current]
            parts.append(name)
            current = parent
        return "".join(reversed(parts))

    folder_count = struct.unpack_from("<I", pamt, offset)[0]
    offset += 8 + folder_count * 16
    entries: list[PazEntry] = []
    while offset + 20 <= len(pamt):
        node_reference, paz_offset, stored_size, original_size, flags = struct.unpack_from(
            "<IIIII", pamt, offset
        )
        offset += 20
        path = build_path(node_reference)
        if folder_prefix:
            path = f"{folder_prefix}/{path}"
        entries.append(PazEntry(path, paz_offset, stored_size, original_size, flags))
    return entries


def build_two_file_overlay(
    template_pamt: bytes,
    first_payload: bytes,
    second_payload: bytes,
) -> tuple[bytes, bytes]:
    """Build the known two-file 0036 overlay used by the Alden mod."""
    parsed_template = parse_pamt(template_pamt)
    expected_paths = ["gamedata/storeinfo.pabgb", "gamedata/storeinfo.pabgh"]
    if [entry.path for entry in parsed_template] != expected_paths:
        raise RuntimeError("Unexpected Alden PAMT template paths")

    paz = bytearray(first_payload)
    paz += b"\x00" * ((-len(paz)) % 16)
    second_offset = len(paz)
    paz += second_payload
    paz += b"\x00" * ((-len(paz)) % 16)
    paz_bytes = bytes(paz)

    pamt = bytearray(template_pamt)
    if len(parsed_template) != 2:
        raise RuntimeError("Alden template must have two records")
    record_offset = len(pamt) - 40
    struct.pack_into("<I", pamt, 8, 0)
    struct.pack_into("<I", pamt, 20, len(paz_bytes))
    node_a = struct.unpack_from("<I", pamt, record_offset)[0]
    node_b = struct.unpack_from("<I", pamt, record_offset + 20)[0]
    struct.pack_into("<IIIII", pamt, record_offset, node_a, 0, len(first_payload), len(first_payload), 0)
    struct.pack_into(
        "<IIIII", pamt, record_offset + 20, node_b, second_offset, len(second_payload), len(second_payload), 0
    )
    struct.pack_into("<I", pamt, 16, pearl_abyss_checksum(paz_bytes))
    struct.pack_into("<I", pamt, 0, pearl_abyss_checksum(bytes(pamt[12:])))
    pamt_bytes = bytes(pamt)

    entries = parse_pamt(pamt_bytes)
    if [entry.path for entry in entries] != expected_paths:
        raise RuntimeError("Generated Alden PAMT paths changed")
    for entry, expected in zip(entries, (first_payload, second_payload)):
        if entry.flags != 0 or entry.stored_size != entry.original_size:
            raise RuntimeError("Generated Alden overlay is unexpectedly compressed or encrypted")
        if paz_bytes[entry.offset:entry.offset + entry.stored_size] != expected:
            raise RuntimeError(f"Generated Alden payload mismatch: {entry.path}")
    return pamt_bytes, paz_bytes


def read_overlay_file(pamt_path: Path, paz_path: Path, wanted_name: str) -> bytes:
    pamt = pamt_path.read_bytes()
    paz = paz_path.read_bytes()
    matches = [entry for entry in parse_pamt(pamt) if Path(entry.path).name == wanted_name]
    if len(matches) != 1:
        raise RuntimeError(f"Expected one {wanted_name} in overlay")
    entry = matches[0]
    if entry.flags != 0 or entry.stored_size != entry.original_size:
        raise RuntimeError("This helper only supports uncompressed, unencrypted overlays")
    return paz[entry.offset:entry.offset + entry.stored_size]

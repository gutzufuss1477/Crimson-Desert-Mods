from __future__ import annotations

import argparse
import hashlib
import hmac
import os
import shutil
import struct
import sys
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path

try:
    import lz4.block
except ImportError:
    lz4 = None

APP_NAME = "Bank Refresh Save Patcher"
APP_VERSION = "1.3.0"
DEFAULT_DELTA_MS = 225_900_000
TARGET_REMAINING_MS = 20 * 60 * 1000
DEFAULT_SAVE_ROOT = Path(os.environ.get("LOCALAPPDATA", "")) / "Pearl Abyss" / "CD" / "save"

SAVE_MAGIC = b"SAVE"
RAW_MAGIC = b"\xFF\xFF\x04\x00"
HEADER_SIZE = 0x80
VERSION_OFFSET = 0x04
FLAGS_OFFSET = 0x06
UNCOMP_SIZE_OFFSET = 0x12
PAYLOAD_SIZE_OFFSET = 0x16
NONCE_OFFSET = 0x1A
HMAC_OFFSET = 0x2A
PAYLOAD_OFFSET = 0x80

_SAVE_BASE_KEY = bytes.fromhex(
    "C41B8E730DF259A637CC04E9B12F9668DA107A853E61F9224DB80AD75C13EF90"
)[:31]
_VERSION_PREFIXES = {
    1: b'^Qgbrm/.#@`zsr]\\@rvfal#"',
    2: b"^Pearl--#Abyss__@!!",
}

TARGET_TYPE = "InventoryItemContentsSaveData"
BANK_DATA_TYPE = "InventoryItemContentsSaveData_BankData"
LEGACY_TIMER = "_lastUpdateBankTime"
BANK_LIST = "_bankDataList"
NEXT_TIMER = "_nextUpdateBankTime"
BANK_KEY = "_bankInfoKey"
TRANSFORM_TYPE = "TransformSaveData"
FIELD_SAVE_TYPE = "TransformFieldSaveData"
FIELD_SAVE_LIST = "_fieldSaveDataList"
LAST_FIELD_KEY = "_lastfieldInfoKey"
FIELD_KEY = "_fieldInfoKey"
FIELD_TIME = "_fieldTimeRaw"


class PatchError(RuntimeError):
    pass


@dataclass
class FieldDef:
    index: int
    name: str
    type_name: str
    meta_kind: int
    meta_size: int
    meta_aux: int


@dataclass
class TypeDef:
    index: int
    name: str
    fields: list[FieldDef]


@dataclass
class TocEntry:
    index: int
    class_index: int
    class_name: str
    data_offset: int
    data_size: int


@dataclass
class ParsedField:
    definition: FieldDef
    present: bool
    start: int | None = None
    end: int | None = None
    value: int | bytes | str | None = None
    elements: list["ParsedObject"] = field(default_factory=list)
    child: "ParsedObject | None" = None


@dataclass
class ParsedObject:
    type_def: TypeDef
    mask: bytes
    start: int
    end: int
    fields: list[ParsedField]

    def get(self, name: str) -> ParsedField | None:
        for item in self.fields:
            if item.definition.name == name:
                return item
        return None


@dataclass
class SaveContainer:
    original_header: bytes
    version: int
    raw: bytearray


@dataclass
class PatchTarget:
    mode: str
    offset: int
    old_value: int
    new_value: int
    bank_key: int | None = None


def _need(condition: bool, message: str) -> None:
    if not condition:
        raise PatchError(message)


def _bounds(blob: bytes | bytearray, offset: int, size: int, label: str = "data") -> None:
    if offset < 0 or size < 0 or offset + size > len(blob):
        raise PatchError(f"{label} is outside the save data.")


def _u16(blob: bytes | bytearray, offset: int) -> int:
    _bounds(blob, offset, 2)
    return struct.unpack_from("<H", blob, offset)[0]


def _u24(blob: bytes | bytearray, offset: int) -> int:
    _bounds(blob, offset, 3)
    return blob[offset] | (blob[offset + 1] << 8) | (blob[offset + 2] << 16)


def _u16be(blob: bytes | bytearray, offset: int) -> int:
    _bounds(blob, offset, 2)
    return (blob[offset] << 8) | blob[offset + 1]


def _u32(blob: bytes | bytearray, offset: int) -> int:
    _bounds(blob, offset, 4)
    return struct.unpack_from("<I", blob, offset)[0]


def _u64(blob: bytes | bytearray, offset: int) -> int:
    _bounds(blob, offset, 8)
    return struct.unpack_from("<Q", blob, offset)[0]


def _generate_save_key(version: int) -> bytes:
    prefix = _VERSION_PREFIXES.get(version)
    if prefix is None:
        raise PatchError(f"Unsupported Crimson Desert save container version: {version}")
    material = prefix + b"PRIVATE_HMAC_SECRET_CHECK"
    return bytes(a ^ b for a, b in zip(_SAVE_BASE_KEY, material)) + b"\x00"


def _rotl32(value: int, shift: int) -> int:
    value &= 0xFFFFFFFF
    return ((value << shift) | (value >> (32 - shift))) & 0xFFFFFFFF


def _quarter_round(state: list[int], a: int, b: int, c: int, d: int) -> None:
    state[a] = (state[a] + state[b]) & 0xFFFFFFFF
    state[d] ^= state[a]
    state[d] = _rotl32(state[d], 16)
    state[c] = (state[c] + state[d]) & 0xFFFFFFFF
    state[b] ^= state[c]
    state[b] = _rotl32(state[b], 12)
    state[a] = (state[a] + state[b]) & 0xFFFFFFFF
    state[d] ^= state[a]
    state[d] = _rotl32(state[d], 8)
    state[c] = (state[c] + state[d]) & 0xFFFFFFFF
    state[b] ^= state[c]
    state[b] = _rotl32(state[b], 7)


def _chacha20_block(key_words: list[int], counter: int, nonce_words: list[int]) -> bytes:
    initial = [
        0x61707865, 0x3320646E, 0x79622D32, 0x6B206574,
        *key_words,
        counter & 0xFFFFFFFF,
        *nonce_words,
    ]
    work = list(initial)
    for _ in range(10):
        _quarter_round(work, 0, 4, 8, 12)
        _quarter_round(work, 1, 5, 9, 13)
        _quarter_round(work, 2, 6, 10, 14)
        _quarter_round(work, 3, 7, 11, 15)
        _quarter_round(work, 0, 5, 10, 15)
        _quarter_round(work, 1, 6, 11, 12)
        _quarter_round(work, 2, 7, 8, 13)
        _quarter_round(work, 3, 4, 9, 14)
    return struct.pack("<16I", *[((work[i] + initial[i]) & 0xFFFFFFFF) for i in range(16)])


def _chacha20_crypt(data: bytes, nonce16: bytes, key: bytes) -> bytes:
    _need(len(key) == 32, "Internal key length error.")
    _need(len(nonce16) == 16, "Invalid save nonce length.")
    counter = struct.unpack_from("<I", nonce16, 0)[0]
    key_words = list(struct.unpack("<8I", key))
    nonce_words = list(struct.unpack("<3I", nonce16[4:]))
    output = bytearray(len(data))
    pos = 0
    while pos < len(data):
        block = _chacha20_block(key_words, counter, nonce_words)
        chunk = data[pos:pos + 64]
        for idx, byte in enumerate(chunk):
            output[pos + idx] = byte ^ block[idx]
        pos += len(chunk)
        counter = (counter + 1) & 0xFFFFFFFF
    return bytes(output)


def _load_container_bytes(file_data: bytes) -> SaveContainer:
    if lz4 is None:
        raise PatchError("Python package 'lz4' is missing. Run Crimson_Bank_Patcher.bat again.")
    _need(len(file_data) >= HEADER_SIZE + 1, "File is too small to be a Crimson Desert save.")
    _need(file_data[:4] == SAVE_MAGIC, "Selected file does not start with the Crimson Desert SAVE header.")
    version = _u16(file_data, VERSION_OFFSET)
    key = _generate_save_key(version)
    uncomp_size = _u32(file_data, UNCOMP_SIZE_OFFSET)
    payload_size = _u32(file_data, PAYLOAD_SIZE_OFFSET)
    _need(uncomp_size > 0, "Save reports an invalid uncompressed size.")
    _need(payload_size > 0, "Save reports an invalid payload size.")
    _need(PAYLOAD_OFFSET + payload_size <= len(file_data), "Save payload size exceeds the file size.")
    nonce = file_data[NONCE_OFFSET:NONCE_OFFSET + 16]
    stored_hmac = file_data[HMAC_OFFSET:HMAC_OFFSET + 32]
    ciphertext = file_data[PAYLOAD_OFFSET:PAYLOAD_OFFSET + payload_size]
    compressed = _chacha20_crypt(ciphertext, nonce, key)
    calculated_hmac = hmac.new(key, compressed, hashlib.sha256).digest()
    _need(hmac.compare_digest(calculated_hmac, stored_hmac), "Save HMAC validation failed. No save was modified.")
    try:
        decompressed = lz4.block.decompress(compressed, uncompressed_size=uncomp_size)
    except Exception as exc:
        raise PatchError(f"LZ4 decompression failed: {exc}") from exc
    _need(len(decompressed) == uncomp_size, "Decompressed save size does not match the container header.")
    _need(decompressed.startswith(RAW_MAGIC), "Decrypted payload is not a recognised Crimson Desert serializer stream.")
    return SaveContainer(file_data[:HEADER_SIZE], version, bytearray(decompressed))


def load_container(path: Path) -> SaveContainer:
    try:
        return _load_container_bytes(path.read_bytes())
    except OSError as exc:
        raise PatchError(f"Could not read save file: {exc}") from exc


def build_container(container: SaveContainer, edited_raw: bytes | bytearray) -> bytes:
    if lz4 is None:
        raise PatchError("Python package 'lz4' is missing.")
    key = _generate_save_key(container.version)
    try:
        compressed = lz4.block.compress(
            bytes(edited_raw), store_size=False, mode="high_compression", compression=9
        )
    except Exception as exc:
        raise PatchError(f"LZ4 compression failed: {exc}") from exc
    nonce = os.urandom(16)
    digest = hmac.new(key, compressed, hashlib.sha256).digest()
    encrypted = _chacha20_crypt(compressed, nonce, key)
    header = bytearray(container.original_header)
    _need(len(header) == HEADER_SIZE, "Invalid original save header size.")
    header[:4] = SAVE_MAGIC
    struct.pack_into("<H", header, VERSION_OFFSET, container.version)
    struct.pack_into("<I", header, UNCOMP_SIZE_OFFSET, len(edited_raw))
    struct.pack_into("<I", header, PAYLOAD_SIZE_OFFSET, len(compressed))
    header[NONCE_OFFSET:NONCE_OFFSET + 16] = nonce
    header[HMAC_OFFSET:HMAC_OFFSET + 32] = digest
    return bytes(header) + encrypted


def parse_schema(raw: bytes | bytearray) -> tuple[list[TypeDef], int]:
    _need(raw.startswith(RAW_MAGIC), "Serializer magic is missing.")
    pos = 0x0E
    _bounds(raw, pos, 6, "serializer schema header")
    pos += 4
    type_count = _u16(raw, pos)
    pos += 2
    _need(1 <= type_count <= 4096, f"Implausible serializer type count: {type_count}")
    root_len = _u32(raw, pos)
    pos += 4
    _need(1 <= root_len <= 1024, "Invalid root type name length.")
    _bounds(raw, pos, root_len, "root type name")
    try:
        current_name = bytes(raw[pos:pos + root_len]).decode("ascii")
    except UnicodeDecodeError as exc:
        raise PatchError("Root type name is not ASCII.") from exc
    pos += root_len
    types: list[TypeDef] = []
    for type_index in range(type_count):
        field_count = _u16(raw, pos)
        pos += 2
        _need(field_count <= 4096, "Implausible field count in serializer schema.")
        fields: list[FieldDef] = []
        for field_index in range(field_count):
            fn_len = _u32(raw, pos)
            pos += 4
            _need(fn_len <= 4096, "Invalid field-name length in serializer schema.")
            _bounds(raw, pos, fn_len, "field name")
            try:
                field_name = bytes(raw[pos:pos + fn_len]).decode("ascii")
            except UnicodeDecodeError as exc:
                raise PatchError("Field name is not ASCII.") from exc
            pos += fn_len
            tn_len = _u32(raw, pos)
            pos += 4
            _need(tn_len <= 4096, "Invalid field type-name length in serializer schema.")
            _bounds(raw, pos, tn_len, "field type name")
            try:
                type_name = bytes(raw[pos:pos + tn_len]).decode("ascii")
            except UnicodeDecodeError as exc:
                raise PatchError("Field type name is not ASCII.") from exc
            pos += tn_len
            _bounds(raw, pos, 8, "field metadata")
            meta_kind = _u16(raw, pos)
            meta_size = _u16(raw, pos + 2)
            meta_aux = _u32(raw, pos + 4)
            pos += 8
            fields.append(FieldDef(field_index, field_name, type_name, meta_kind, meta_size, meta_aux))
        types.append(TypeDef(type_index, current_name, fields))
        if type_index != type_count - 1:
            name_len = _u32(raw, pos)
            pos += 4
            _need(1 <= name_len <= 4096, "Invalid serializer type-name length.")
            _bounds(raw, pos, name_len, "serializer type name")
            try:
                current_name = bytes(raw[pos:pos + name_len]).decode("ascii")
            except UnicodeDecodeError as exc:
                raise PatchError("Serializer type name is not ASCII.") from exc
            pos += name_len
    return types, pos


def parse_toc(raw: bytes | bytearray, schema_end: int, types: list[TypeDef]) -> list[TocEntry]:
    _bounds(raw, schema_end, 12, "serializer TOC header")
    toc_count = _u32(raw, schema_end + 4)
    stream_size = _u32(raw, schema_end + 8)
    _need(1 <= toc_count <= 100000, f"Implausible serializer TOC count: {toc_count}")
    _need(stream_size <= len(raw), "Serializer stream size exceeds the decrypted payload.")
    names = {item.index: item.name for item in types}
    entries: list[TocEntry] = []
    start = schema_end + 12
    _bounds(raw, start, toc_count * 20, "serializer TOC")
    for index in range(toc_count):
        off = start + index * 20
        class_index, _s1, _s2, data_offset, data_size = struct.unpack_from("<5I", raw, off)
        class_name = names.get(class_index, f"<class_{class_index}>")
        _need(data_offset + data_size <= len(raw), "A serializer object points outside the decrypted save.")
        entries.append(TocEntry(index, class_index, class_name, data_offset, data_size))
    return entries


def field_present(mask: bytes, field_index: int) -> bool:
    byte_index = field_index // 8
    bit_index = field_index % 8
    return byte_index < len(mask) and (mask[byte_index] & (1 << bit_index)) != 0


def parse_fixed(raw: bytes | bytearray, cursor: int, field_def: FieldDef, tail: int) -> tuple[int, int | bytes]:
    size = field_def.meta_size
    _need(size > 0, f"Field {field_def.name} has no fixed size.")
    _need(cursor + size <= tail, f"Field {field_def.name} overruns its object.")
    data = bytes(raw[cursor:cursor + size])
    if size in (1, 2, 4, 8):
        return cursor + size, int.from_bytes(data, "little", signed=False)
    return cursor + size, data


def parse_inline_bytes(raw: bytes | bytearray, cursor: int, field_def: FieldDef, tail: int) -> tuple[int, str | bytes]:
    _need(cursor + 4 <= tail, f"Field {field_def.name} overruns its object.")
    count = _u32(raw, cursor)
    total = 4 + count * field_def.meta_size
    _need(field_def.meta_size > 0 and cursor + total <= tail, f"Field {field_def.name} has an invalid inline length.")
    data = bytes(raw[cursor + 4:cursor + total])
    lower = field_def.type_name.lower()
    if field_def.meta_size == 1 and ("string" in lower or lower.endswith("a")):
        return cursor + total, data.rstrip(b"\x00").decode("utf-8", errors="replace")
    return cursor + total, data


def parse_dynamic_array(raw: bytes | bytearray, cursor: int, field_def: FieldDef, tail: int) -> tuple[int, bytes]:
    size = field_def.meta_size
    _need(size > 0, f"Dynamic field {field_def.name} has zero element size.")
    if cursor + 14 <= tail and bytes(raw[cursor:cursor + 5]) == b"\x00\x00\x06\x01\x00":
        count = _u32(raw, cursor + 5)
        end = cursor + 9 + count * size + 5
        if count < 0x10000 and end <= tail and bytes(raw[end - 5:end]) == b"\x01" * 5:
            return end, bytes(raw[cursor:end])
    if cursor < tail and raw[cursor] == 1:
        marker_end = cursor
        while marker_end < tail and raw[marker_end] == 1:
            marker_end += 1
        if marker_end > cursor and marker_end + 5 <= tail and raw[marker_end] == 0:
            count = _u32(raw, marker_end + 1)
            end = marker_end + 5 + count * size
            if count < 0x10000 and end <= tail:
                if end < tail and raw[end] == 1:
                    end += 1
                return end, bytes(raw[cursor:end])
    if cursor + 6 <= tail and raw[cursor] == 0 and raw[cursor + 1] == 0 and raw[cursor + 4] == 0 and raw[cursor + 5] == 0:
        count = _u16(raw, cursor + 2)
        data_start = cursor + 6
    else:
        _need(cursor + 5 <= tail, f"Dynamic field {field_def.name} has an invalid header.")
        count = _u32(raw, cursor + 1)
        data_start = cursor + 5
    _need(count <= 100000, f"Dynamic field {field_def.name} has an implausible element count.")
    end = data_start + count * size
    _need(end <= tail, f"Dynamic field {field_def.name} overruns its object.")
    return end, bytes(raw[cursor:end])


def parse_inline_payload(
    raw: bytes | bytearray,
    type_def: TypeDef,
    mask: bytes,
    payload_start: int,
    tail: int,
    type_map: dict[int, TypeDef],
    depth: int,
) -> tuple[int, ParsedObject]:
    _need(depth <= 24, "Serializer object nesting is unexpectedly deep.")
    _bounds(raw, payload_start, 4, "inline object payload")
    cursor = payload_start + 4
    parsed: list[ParsedField] = []
    for definition in type_def.fields:
        present = field_present(mask, definition.index)
        item = ParsedField(definition, present)
        parsed.append(item)
        if not present:
            continue
        item.start = cursor
        if definition.meta_kind in (0, 2):
            cursor, item.value = parse_fixed(raw, cursor, definition, tail)
        elif definition.meta_kind == 1:
            cursor, item.value = parse_inline_bytes(raw, cursor, definition, tail)
        elif definition.meta_kind == 3:
            cursor, item.value = parse_dynamic_array(raw, cursor, definition, tail)
        elif definition.meta_kind in (4, 5):
            cursor, child = parse_object_locator(raw, cursor, tail, type_map, definition.meta_kind, depth + 1)
            item.child = child
        elif definition.meta_kind in (6, 7):
            cursor, elements = parse_object_list(raw, cursor, tail, type_map, depth + 1)
            item.elements = elements
        else:
            raise PatchError(f"Unsupported serializer field kind {definition.meta_kind} in {type_def.name}.{definition.name}.")
        item.end = cursor
    _need(cursor + 4 <= tail, f"Inline object {type_def.name} is missing its trailing size.")
    size_offset = -1
    for probe in range(cursor, min(tail - 4, cursor + 16) + 1):
        if _u32(raw, probe) == probe - payload_start:
            gap = bytes(raw[cursor:probe])
            if all(byte in (0x00, 0x01) for byte in gap):
                size_offset = probe
                break
    _need(size_offset >= 0, f"Inline object {type_def.name} has an unexpected trailing size.")
    end = size_offset + 4
    return end, ParsedObject(type_def, mask, payload_start, end, parsed)


def parse_object_locator(
    raw: bytes | bytearray,
    cursor: int,
    tail: int,
    type_map: dict[int, TypeDef],
    locator_kind: int,
    depth: int,
) -> tuple[int, ParsedObject | None]:
    body = cursor
    if locator_kind == 5:
        body = -1
        for delta in range(9):
            probe = cursor + delta
            if probe + 2 <= tail:
                count = _u16(raw, probe)
                if 0 < count <= 16:
                    body = probe
                    break
        _need(body >= 0, "Could not locate an object-pointer payload.")
    _need(body + 18 <= tail, "Object locator overruns its parent object.")
    mask_count = _u16(raw, body)
    _need(0 < mask_count <= 16, "Object locator has an invalid mask size.")
    _bounds(raw, body + 2, mask_count + 15, "object locator")
    mask = bytes(raw[body + 2:body + 2 + mask_count])
    type_index = _u16(raw, body + 2 + mask_count)
    sentinel1 = _u32(raw, body + 2 + mask_count + 3)
    sentinel2 = _u32(raw, body + 2 + mask_count + 7)
    payload_offset = _u32(raw, body + 2 + mask_count + 11)
    wrapper_end = body + 2 + mask_count + 15
    child_type = type_map.get(type_index)
    _need(child_type is not None, "Object locator references an unknown serializer type.")
    if payload_offset == wrapper_end:
        child_end, child = parse_inline_payload(raw, child_type, mask, payload_offset, tail, type_map, depth + 1)
        return child_end, child
    _need(sentinel1 in (0, 0xFFFFFFFF) and sentinel2 in (0, 0xFFFFFFFF), "Object locator sentinel values are unexpected.")
    return wrapper_end, None


def parse_compact_element(
    raw: bytes | bytearray,
    cursor: int,
    tail: int,
    type_map: dict[int, TypeDef],
    depth: int,
) -> tuple[int, ParsedObject]:
    _need(cursor + 18 <= tail, "Compact list element overruns its list.")
    mask_count = _u16(raw, cursor)
    _need(1 <= mask_count <= 16, "Compact list element has an invalid mask size.")
    header_size = 2 + mask_count + 2 + 1 + 8 + 4
    _need(cursor + header_size <= tail, "Compact list element header overruns its list.")
    mask = bytes(raw[cursor + 2:cursor + 2 + mask_count])
    type_index = _u16(raw, cursor + 2 + mask_count)
    sentinel_offset = cursor + 2 + mask_count + 3
    _need(_u64(raw, sentinel_offset) == 0xFFFFFFFFFFFFFFFF, "Compact list element sentinel mismatch.")
    payload_offset = _u32(raw, sentinel_offset + 8)
    _need(payload_offset == cursor + header_size, "Compact list element payload is not inline.")
    type_def = type_map.get(type_index)
    _need(type_def is not None, "Compact list element references an unknown serializer type.")
    end, child = parse_inline_payload(raw, type_def, mask, payload_offset, tail, type_map, depth + 1)
    return end, child


def parse_list_element(
    raw: bytes | bytearray,
    cursor: int,
    tail: int,
    type_map: dict[int, TypeDef],
    depth: int,
) -> tuple[int, ParsedObject]:
    first_error: Exception | None = None
    try:
        end, child = parse_object_locator(raw, cursor, tail, type_map, 4, depth + 1)
        if child is not None:
            return end, child
    except Exception as exc:
        first_error = exc
    try:
        return parse_compact_element(raw, cursor, tail, type_map, depth + 1)
    except Exception as exc:
        if first_error is not None:
            raise PatchError(f"Object-list element could not be decoded safely: {first_error}; {exc}") from exc
        raise


def parse_object_list(
    raw: bytes | bytearray,
    cursor: int,
    tail: int,
    type_map: dict[int, TypeDef],
    depth: int,
) -> tuple[int, list[ParsedObject]]:
    _need(depth <= 24, "Serializer list nesting is unexpectedly deep.")
    best: tuple[int, list[ParsedObject]] | None = None
    errors: list[str] = []
    for body in (cursor, cursor + 1, cursor + 2, cursor + 3):
        try:
            _need(body + 18 <= tail, "Object-list header overruns its parent object.")
            prefix = raw[body]
            marker_end = body
            while marker_end < tail and raw[marker_end] == 1:
                marker_end += 1
            if marker_end > body and marker_end + 17 <= tail and raw[marker_end] == 0 and bytes(raw[marker_end + 5:marker_end + 18]) == b"\x00" * 13:
                count = _u32(raw, marker_end + 1)
                header_size = (marker_end - body + 1) + 4 + 13
            elif prefix == 0 and bytes(raw[body + 1:body + 4]) == b"\x00\x00\x00":
                count = _u32(raw, body + 4)
                header_size = 18
            elif prefix == 0:
                count = _u24(raw, body + 1)
                header_size = 18
            elif prefix == 1 and body + 21 <= tail and bytes(raw[body + 1:body + 4]) == b"\x01\x01\x00":
                count = _u32(raw, body + 4)
                header_size = 21
            elif prefix == 1:
                count = _u16be(raw, body + 1)
                header_size = 19
            else:
                raise PatchError(f"Unsupported object-list prefix 0x{prefix:02X}.")
            _need(count <= 100000, "Object-list element count is implausible.")
            element_cursor = body + header_size
            elements: list[ParsedObject] = []
            for _ in range(count):
                element_cursor, child = parse_list_element(raw, element_cursor, tail, type_map, depth + 1)
                elements.append(child)
            _need(len(elements) == count, "Object list was only partially decoded.")
            candidate = (element_cursor, elements)
            if best is None or candidate[0] > best[0]:
                best = candidate
        except Exception as exc:
            errors.append(str(exc))
    if best is None:
        detail = errors[-1] if errors else "unknown list layout"
        raise PatchError(f"Could not decode an object list safely: {detail}")
    return best


def parse_top_object(
    raw: bytes | bytearray,
    entry: TocEntry,
    type_def: TypeDef,
    type_map: dict[int, TypeDef],
) -> ParsedObject:
    start = entry.data_offset
    end = entry.data_offset + entry.data_size
    _need(start + 2 <= end, f"{type_def.name} object is too small.")
    mask_count = _u16(raw, start)
    expected_mask_count = max(1, (len(type_def.fields) + 7) // 8)
    _need(mask_count == expected_mask_count, f"{type_def.name} mask size changed ({mask_count}, expected {expected_mask_count}).")
    _bounds(raw, start + 2, mask_count + 4, f"{type_def.name} header")
    mask = bytes(raw[start + 2:start + 2 + mask_count])
    cursor = start + 2 + mask_count + 4
    parsed: list[ParsedField] = []
    for definition in type_def.fields:
        present = field_present(mask, definition.index)
        item = ParsedField(definition, present)
        parsed.append(item)
        if not present:
            continue
        item.start = cursor
        if definition.meta_kind in (0, 2):
            cursor, item.value = parse_fixed(raw, cursor, definition, end)
        elif definition.meta_kind == 1:
            cursor, item.value = parse_inline_bytes(raw, cursor, definition, end)
        elif definition.meta_kind == 3:
            cursor, item.value = parse_dynamic_array(raw, cursor, definition, end)
        elif definition.meta_kind in (4, 5):
            cursor, child = parse_object_locator(raw, cursor, end, type_map, definition.meta_kind, 1)
            item.child = child
        elif definition.meta_kind in (6, 7):
            cursor, elements = parse_object_list(raw, cursor, end, type_map, 1)
            item.elements = elements
        else:
            raise PatchError(f"Unsupported serializer field kind {definition.meta_kind} in {type_def.name}.{definition.name}.")
        item.end = cursor
    _need(cursor == end, f"Could not account for the complete {type_def.name} object ({end - cursor} bytes remain).")
    return ParsedObject(type_def, mask, start, end, parsed)


def parse_target(raw: bytes | bytearray) -> tuple[ParsedObject, dict[int, TypeDef]]:
    types, schema_end = parse_schema(raw)
    type_map = {item.index: item for item in types}
    by_name = {item.name: item for item in types}
    target_type = by_name.get(TARGET_TYPE)
    _need(target_type is not None, f"Save schema does not contain {TARGET_TYPE}.")
    entries = parse_toc(raw, schema_end, types)
    matches = [item for item in entries if item.class_index == target_type.index]
    _need(len(matches) == 1, f"Expected exactly one {TARGET_TYPE} object, found {len(matches)}.")
    target = parse_top_object(raw, matches[0], target_type, type_map)
    return target, type_map


def locate_current_field_time(raw: bytes | bytearray) -> int:
    """Locate the current saved world/field clock used by the bank timer."""
    types, schema_end = parse_schema(raw)
    type_map = {item.index: item for item in types}
    by_name = {item.name: item for item in types}
    transform_type = by_name.get(TRANSFORM_TYPE)
    _need(transform_type is not None, f"Save schema does not contain {TRANSFORM_TYPE}.")
    entries = parse_toc(raw, schema_end, types)
    matches = [item for item in entries if item.class_index == transform_type.index]
    _need(len(matches) == 1, f"Expected exactly one {TRANSFORM_TYPE} object, found {len(matches)}.")
    transform = parse_top_object(raw, matches[0], transform_type, type_map)

    last_key = transform.get(LAST_FIELD_KEY)
    _need(last_key is not None and last_key.present and isinstance(last_key.value, int),
          f"Could not safely read {TRANSFORM_TYPE}.{LAST_FIELD_KEY}.")
    current_key = int(last_key.value)

    field_list = transform.get(FIELD_SAVE_LIST)
    _need(field_list is not None and field_list.present,
          f"Could not safely locate {TRANSFORM_TYPE}.{FIELD_SAVE_LIST}.")
    _need(field_list.definition.meta_kind in (6, 7),
          f"{TRANSFORM_TYPE}.{FIELD_SAVE_LIST} is no longer an object list.")

    candidates: list[int] = []
    for field_save in field_list.elements:
        _need(field_save.type_def.name == FIELD_SAVE_TYPE,
              f"{FIELD_SAVE_LIST} contains unexpected type {field_save.type_def.name}.")
        key = field_save.get(FIELD_KEY)
        if key is None or not key.present or not isinstance(key.value, int) or int(key.value) != current_key:
            continue
        timer = field_save.get(FIELD_TIME)
        _need(timer is not None and timer.present,
              f"Current {FIELD_SAVE_TYPE} does not serialize {FIELD_TIME}.")
        validate_u64_field(timer, f"{FIELD_SAVE_TYPE}.{FIELD_TIME}")
        candidates.append(int(timer.value))

    _need(len(candidates) == 1,
          f"Expected exactly one current {FIELD_SAVE_TYPE}.{FIELD_TIME}, found {len(candidates)}.")
    _need(candidates[0] > 0, f"Current {FIELD_TIME} is not plausible.")
    return candidates[0]


def validate_u64_field(item: ParsedField, qualified_name: str) -> None:
    definition = item.definition
    _need(definition.meta_kind in (0, 2), f"{qualified_name} is no longer a scalar field.")
    _need(definition.meta_size == 8, f"{qualified_name} is no longer 8 bytes wide.")
    _need(item.start is not None and item.end == item.start + 8, f"{qualified_name} could not be located exactly.")
    _need(isinstance(item.value, int), f"{qualified_name} did not decode as an integer.")


def locate_patch_targets(raw: bytes | bytearray, delta_ms: int = DEFAULT_DELTA_MS) -> tuple[str, list[PatchTarget]]:
    _need(delta_ms > 0, "Patch delta must be positive.")
    target, _type_map = parse_target(raw)
    legacy = target.get(LEGACY_TIMER)
    if legacy is not None and legacy.present:
        validate_u64_field(legacy, f"{TARGET_TYPE}.{LEGACY_TIMER}")
        old = int(legacy.value)
        _need(old > delta_ms, f"{LEGACY_TIMER} is too small for the configured patch delta.")
        return "legacy-global-timer", [PatchTarget("legacy", legacy.start, old, old - delta_ms, None)]

    current_field_time = locate_current_field_time(raw)
    desired_next_time = current_field_time + TARGET_REMAINING_MS

    bank_list = target.get(BANK_LIST)
    _need(bank_list is not None, f"{TARGET_TYPE} has neither a present {LEGACY_TIMER} nor a {BANK_LIST} field.")
    _need(bank_list.present, f"{TARGET_TYPE}.{LEGACY_TIMER} is absent and {BANK_LIST} is not serialized.")
    _need(bank_list.definition.meta_kind in (6, 7), f"{TARGET_TYPE}.{BANK_LIST} is no longer an object list.")
    _need(bank_list.elements, f"{TARGET_TYPE}.{BANK_LIST} is empty; no active bank timer can be patched safely.")

    found: list[PatchTarget] = []
    for bank in bank_list.elements:
        _need(bank.type_def.name == BANK_DATA_TYPE, f"{BANK_LIST} contains unexpected type {bank.type_def.name}.")
        key_field = bank.get(BANK_KEY)
        bank_key: int | None = None
        if key_field is not None and key_field.present and isinstance(key_field.value, int):
            bank_key = key_field.value
        timer = bank.get(NEXT_TIMER)
        _need(timer is not None, f"{BANK_DATA_TYPE} no longer contains {NEXT_TIMER}.")
        if not timer.present:
            continue
        validate_u64_field(timer, f"{BANK_DATA_TYPE}.{NEXT_TIMER}")
        old = int(timer.value)
        _need(old > 0, f"Bank {bank_key}: {NEXT_TIMER} is not plausible.")
        # Never push a refresh farther into the future. If it is already due in
        # 20 minutes or less, leave the existing timestamp unchanged.
        new_value = min(old, desired_next_time)
        found.append(PatchTarget("per-bank", timer.start, old, new_value, bank_key))

    _need(found, f"No serialized {BANK_DATA_TYPE}.{NEXT_TIMER} value was found.")
    offsets = [item.offset for item in found]
    _need(len(offsets) == len(set(offsets)), "Bank timer detection produced duplicate offsets.")
    _need(len(found) <= 32, f"Implausible number of active bank timers: {len(found)}")
    return "v1.16+-per-bank-next-timer", found


def apply_targets(raw: bytearray, targets: list[PatchTarget]) -> bytearray:
    edited = bytearray(raw)
    for item in targets:
        _bounds(edited, item.offset, 8, "bank timer")
        current = _u64(edited, item.offset)
        _need(current == item.old_value, "Bank timer changed between detection and patching.")
        struct.pack_into("<Q", edited, item.offset, item.new_value)
    return edited


def verify_edited_raw(original_raw: bytearray, edited_raw: bytearray, targets: list[PatchTarget]) -> None:
    _need(len(original_raw) == len(edited_raw), "Patch unexpectedly changed the serializer size.")
    mode2, targets2 = locate_patch_targets(edited_raw)
    _need(len(targets2) == len(targets), "Patched save no longer exposes the same timer structure.")
    expected = {(item.offset, item.new_value) for item in targets}
    actual = {(item.offset, item.old_value) for item in targets2}
    _need(expected == actual, "Post-patch timer values did not validate.")
    allowed = set()
    for item in targets:
        allowed.update(range(item.offset, item.offset + 8))
    changed = {idx for idx, (before, after) in enumerate(zip(original_raw, edited_raw)) if before != after}
    _need(changed <= allowed, "Patch changed bytes outside the validated bank timer fields.")


def make_backup_path(save_path: Path) -> Path:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    base = save_path.with_name(f"{save_path.name}.bankpatch_backup_{stamp}")
    candidate = base
    counter = 1
    while candidate.exists():
        candidate = save_path.with_name(f"{base.name}_{counter}")
        counter += 1
    return candidate


def patch_save(save_path: Path, dry_run: bool = False) -> tuple[str, list[PatchTarget], Path | None]:
    save_path = save_path.expanduser().resolve()
    _need(save_path.is_file(), f"Save file not found: {save_path}")
    original_file = save_path.read_bytes()
    original_sha = hashlib.sha256(original_file).hexdigest()
    container = _load_container_bytes(original_file)
    mode, targets = locate_patch_targets(container.raw)
    edited_raw = apply_targets(container.raw, targets)
    verify_edited_raw(container.raw, edited_raw, targets)
    rebuilt = build_container(container, edited_raw)
    rebuilt_sha = hashlib.sha256(rebuilt).hexdigest()
    _need(hashlib.sha256(save_path.read_bytes()).hexdigest() == original_sha, "Save file changed while the patch was being prepared. No save was overwritten.")
    if dry_run:
        # Dry-run still performs one complete in-memory round-trip verification.
        roundtrip = _load_container_bytes(rebuilt)
        _need(roundtrip.raw == edited_raw, "Rebuilt SAVE container failed its decrypt/HMAC/LZ4 round-trip check.")
        return mode, targets, None

    backup = make_backup_path(save_path)
    try:
        shutil.copy2(save_path, backup)
    except OSError as exc:
        raise PatchError(f"Could not create backup: {exc}") from exc
    _need(hashlib.sha256(backup.read_bytes()).hexdigest() == original_sha, "Backup verification failed. No save was overwritten.")

    temp_path = save_path.with_name(save_path.name + ".bankpatch_tmp")
    try:
        with open(temp_path, "wb") as handle:
            handle.write(rebuilt)
            handle.flush()
            os.fsync(handle.fileno())
        _need(hashlib.sha256(temp_path.read_bytes()).hexdigest() == rebuilt_sha,
              "Temporary patched file differs from the verified build. Original save was not overwritten.")
        # Perform the expensive decrypt/HMAC/LZ4 round-trip once on the exact
        # temporary file that will replace the save. Final verification can then
        # be a byte-for-byte SHA-256 check after the atomic rename.
        temp_container = load_container(temp_path)
        _need(temp_container.raw == edited_raw, "Temporary patched save failed verification. Original save was not overwritten.")
        os.replace(temp_path, save_path)
        _need(hashlib.sha256(save_path.read_bytes()).hexdigest() == rebuilt_sha,
              "Final patched save differs after replacement.")
    except Exception:
        try:
            if temp_path.exists():
                temp_path.unlink()
        except OSError:
            pass
        if save_path.exists() and hashlib.sha256(save_path.read_bytes()).hexdigest() != original_sha:
            try:
                shutil.copy2(backup, save_path)
            except OSError:
                pass
        raise
    return mode, targets, backup


def best_effort_slot_name(slot_dir: Path) -> str:
    lobby = slot_dir / "lobby.save"
    if not lobby.is_file():
        return ""
    try:
        container = load_container(lobby)
        types, schema_end = parse_schema(container.raw)
        type_map = {item.index: item for item in types}
        by_name = {item.name: item for item in types}
        lobby_type = by_name.get("LobbySaveData")
        if lobby_type is None:
            return ""
        entries = parse_toc(container.raw, schema_end, types)
        matches = [entry for entry in entries if entry.class_index == lobby_type.index]
        if len(matches) != 1:
            return ""
        obj = parse_top_object(container.raw, matches[0], lobby_type, type_map)
        slot_field = obj.get("_slotSaveData")
        if slot_field is None or not slot_field.present or slot_field.child is None:
            return ""
        name_field = slot_field.child.get("_customDisplayName")
        if name_field and name_field.present and isinstance(name_field.value, str):
            return name_field.value.strip()
    except Exception:
        return ""
    return ""


def find_saves(root: Path) -> list[Path]:
    r"""Find saves both directly under save\slot* and under save\<profile>\slot*."""
    if not root.is_dir():
        return []

    saves: list[Path] = []
    seen: set[Path] = set()

    # Older/direct layout: ...\CD\save\slot106\save.save
    # Current layout:      ...\CD\save\8364233\slot106\save.save
    for pattern in ("slot*/save.save", "*/slot*/save.save"):
        for path in root.glob(pattern):
            if path.is_file():
                resolved = path.resolve()
                if resolved not in seen:
                    seen.add(resolved)
                    saves.append(path)

    return sorted(saves, key=lambda item: item.stat().st_mtime, reverse=True)


def save_profile_label(save_path: Path, root: Path) -> str:
    """Return the optional folder between the save root and slot directory."""
    try:
        relative = save_path.parent.relative_to(root)
    except ValueError:
        return ""
    if len(relative.parts) >= 2:
        return relative.parts[-2]
    return ""


def describe_target(item: PatchTarget) -> str:
    if item.bank_key is None:
        return f"{LEGACY_TIMER}: {item.old_value} -> {item.new_value}"
    return f"Bank {item.bank_key} {NEXT_TIMER}: {item.old_value} -> {item.new_value}"


def print_result(save_path: Path, mode: str, targets: list[PatchTarget], backup: Path | None, dry_run: bool) -> None:
    print()
    print("Validation passed.")
    print(f"Detected layout: {mode}")
    for item in targets:
        print("  " + describe_target(item))
    print("Bank history timestamps: unchanged")
    if dry_run:
        print("Dry run only: no file was modified.")
    else:
        print(f"Patched save: {save_path}")
        print(f"Backup: {backup}")
        print("Post-write HMAC/LZ4/serializer verification: PASS")


def choose_save_interactively(root: Path) -> Path | None:
    saves = find_saves(root)
    print(f"Default save folder: {root}")
    if not saves:
        print("No save files were found under the default save folder.")
        print("Expected layouts include save\\slot*\\save.save and save\\<profile>\\slot*\\save.save.")
        manual = input("Enter a full path to save.save, or press Enter to exit: ").strip().strip('"')
        return Path(manual) if manual else None

    print()
    print("1. Select a save slot from the list")
    print("2. Automatically patch the newest save")
    print("3. Exit")
    choice = input("Choose: ").strip()
    if choice == "3" or choice == "":
        return None
    if choice == "2":
        return saves[0]
    if choice != "1":
        raise PatchError("Invalid menu choice.")

    print()
    for index, path in enumerate(saves, start=1):
        slot = path.parent.name
        custom = best_effort_slot_name(path.parent)
        modified = datetime.fromtimestamp(path.stat().st_mtime).strftime("%Y-%m-%d %H:%M:%S")
        label = f" - {custom}" if custom else ""
        profile = save_profile_label(path, root)
        profile_label = f" [profile {profile}]" if profile else ""
        print(f"{index:>2}. {slot}{label}{profile_label} [{modified}]")
    selected = input("Select slot number: ").strip()
    try:
        number = int(selected)
    except ValueError as exc:
        raise PatchError("Invalid slot number.") from exc
    _need(1 <= number <= len(saves), "Slot number is out of range.")
    return saves[number - 1]


def main() -> int:
    parser = argparse.ArgumentParser(description=f"{APP_NAME} {APP_VERSION}")
    parser.add_argument("--file", type=Path, help="Patch a specific save.save file")
    parser.add_argument("--newest", action="store_true", help="Patch the newest save in the default save folder")
    parser.add_argument("--dry-run", action="store_true", help="Validate and show the patch without modifying the save")
    parser.add_argument("--yes", action="store_true", help="Skip the final confirmation for command-line use")
    args = parser.parse_args()

    print(f"{APP_NAME} v{APP_VERSION}")
    print("Supports legacy saves and the Crimson Desert 1.16+ per-bank timer layout.")
    print("For 1.16+ saves, refresh is reduced to about 20 minutes from the saved game clock.")
    print("Safety mode: fail closed on unknown or ambiguous serializer structures.")
    print()

    try:
        if args.file:
            save_path = args.file
        elif args.newest:
            saves = find_saves(DEFAULT_SAVE_ROOT)
            _need(saves, f"No saves found under {DEFAULT_SAVE_ROOT}")
            save_path = saves[0]
        else:
            save_path = choose_save_interactively(DEFAULT_SAVE_ROOT)
            if save_path is None:
                return 0

        save_path = save_path.expanduser().resolve()
        print(f"Selected: {save_path}")
        if not args.dry_run and not args.yes:
            print("Close Crimson Desert or return to the title screen before continuing.")
            confirm = input("Patch this save and create a backup? [y/N]: ").strip().lower()
            if confirm not in ("y", "yes"):
                print("Cancelled. No save was modified.")
                return 0

        mode, targets, backup = patch_save(save_path, dry_run=args.dry_run)
        print_result(save_path, mode, targets, backup, args.dry_run)
        return 0
    except PatchError as exc:
        print()
        print(f"Error: {exc}")
        print("No save was overwritten if the patch did not finish successfully.")
        return 2
    except Exception as exc:
        print()
        print(f"Unexpected error: {exc}")
        print("No save was overwritten if the patch did not finish successfully.")
        return 3


if __name__ == "__main__":
    raise SystemExit(main())

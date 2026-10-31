"""Bounded reader/writer for the name-first StaticInfo tables in game 2.04.00.

Record contents stay opaque. Rebuilding one record preserves every other byte
and the complete ordered index, including records introduced by the update.
"""
from dataclasses import dataclass
import struct


@dataclass(frozen=True)
class Record:
    key: int
    name: str
    start: int
    data: bytes
    prefix_size: int


class Table:
    def __init__(self, body: bytes, header: bytes, key_size: int = 4):
        if key_size not in (2, 4) or len(header) < 2:
            raise ValueError("Invalid table header")
        count = struct.unpack_from("<H", header)[0]
        if len(header) != 2 + count * (key_size + 4):
            raise ValueError("Invalid index length")
        index = []
        for i in range(count):
            pos = 2 + i * (key_size + 4)
            key = int.from_bytes(header[pos:pos + key_size], "little")
            start = struct.unpack_from("<I", header, pos + key_size)[0]
            index.append((key, start))
        if len({key for key, _ in index}) != count:
            raise ValueError("Duplicate record key")
        if (count and index[0][1] != 0) or (not count and body):
            raise ValueError("Unindexed body bytes")
        records = []
        for i, (key, start) in enumerate(index):
            end = index[i + 1][1] if i + 1 < count else len(body)
            if not 0 <= start < end <= len(body) or end - start < 4 + key_size:
                raise ValueError(f"Invalid record bounds: {key}")
            data = body[start:end]
            n = struct.unpack_from("<I", data)[0]
            if not 0 < n <= 1024 or 4 + n + key_size > len(data):
                raise ValueError(f"Invalid name length: {key}")
            name = data[4:4 + n].decode("utf-8", errors="strict")
            if "\0" in name or int.from_bytes(data[4 + n:4 + n + key_size], "little") != key:
                raise ValueError(f"Record/index identity mismatch: {key}")
            records.append(Record(key, name, start, data, 4 + n + key_size))
        self.body, self.header, self.key_size = body, header, key_size
        self.records = tuple(records)
        self.by_key = {r.key: r for r in records}

    def require(self, key: int, name: str) -> Record:
        record = self.by_key.get(key)
        if record is None or record.name != name:
            raise ValueError(f"Required record changed: {key}/{name}")
        return record

    def replace(self, replacements: dict[int, bytes]) -> "Table":
        if not replacements.keys() <= self.by_key.keys():
            raise ValueError("Replacement key is missing from source table")
        body = bytearray()
        header = bytearray(struct.pack("<H", len(self.records)))
        for old in self.records:
            header.extend(old.key.to_bytes(self.key_size, "little"))
            header.extend(struct.pack("<I", len(body)))
            body.extend(replacements.get(old.key, old.data))
        rebuilt = Table(bytes(body), bytes(header), self.key_size)
        for old, new in zip(self.records, rebuilt.records, strict=True):
            if (old.key, old.name) != (new.key, new.name):
                raise ValueError("Replacement changed record identity/order")
            if old.key not in replacements and old.data != new.data:
                raise ValueError(f"Unexpected record change: {old.key}")
        return rebuilt


def unique_offset(data: bytes, needle: bytes) -> int:
    pos = data.find(needle)
    if pos < 0 or data.find(needle, pos + 1) >= 0:
        raise ValueError(f"Required byte context is absent or ambiguous: {needle.hex()}")
    return pos

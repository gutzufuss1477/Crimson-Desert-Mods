from __future__ import annotations

import struct
from dataclasses import dataclass


@dataclass(frozen=True)
class Record:
    key: int
    start: int
    end: int
    name: str

    @property
    def size(self) -> int:
        return self.end - self.start


def parse_u32_header(data: bytes, header: bytes) -> list[Record]:
    if len(header) < 2:
        raise RuntimeError("Header is too short")
    count = struct.unpack_from("<H", header, 0)[0]
    expected = 2 + count * 8
    if len(header) != expected:
        raise RuntimeError(f"Unexpected u32 header size: {len(header)} != {expected}")
    entries = [struct.unpack_from("<II", header, 2 + index * 8) for index in range(count)]
    records: list[Record] = []
    for index, (key, start) in enumerate(entries):
        end = entries[index + 1][1] if index + 1 < len(entries) else len(data)
        if not 0 <= start <= end <= len(data):
            raise RuntimeError(f"Invalid record range for key {key}: {start}:{end}")
        if start + 8 <= end:
            name_length = struct.unpack_from("<I", data, start + 4)[0]
            name_end = start + 8 + name_length
            if name_end > end:
                raise RuntimeError(f"Invalid name length for key {key}")
            name = data[start + 8:name_end].decode("utf-8", errors="replace").rstrip("\x00")
        else:
            name = ""
        records.append(Record(key=key, start=start, end=end, name=name))
    return records


def build_u32_header(records: list[Record], offsets: list[int]) -> bytes:
    if len(records) != len(offsets):
        raise ValueError("records and offsets differ in length")
    header = bytearray(struct.pack("<H", len(records)))
    for record, offset in zip(records, offsets):
        header += struct.pack("<II", record.key, offset)
    return bytes(header)


def parse_store_header(data: bytes, header: bytes) -> list[Record]:
    if len(header) < 2:
        raise RuntimeError("Store header is too short")
    count = struct.unpack_from("<H", header, 0)[0]
    expected = 2 + count * 6
    if len(header) != expected:
        raise RuntimeError(f"Unexpected store header size: {len(header)} != {expected}")
    entries = [struct.unpack_from("<HI", header, 2 + index * 6) for index in range(count)]
    records: list[Record] = []
    for index, (key, start) in enumerate(entries):
        end = entries[index + 1][1] if index + 1 < len(entries) else len(data)
        if not 0 <= start <= end <= len(data):
            raise RuntimeError(f"Invalid store record range for key {key}: {start}:{end}")
        if start + 6 <= end:
            name_length = struct.unpack_from("<I", data, start + 2)[0]
            name_end = start + 6 + name_length
            if name_end > end:
                raise RuntimeError(f"Invalid store name length for key {key}")
            name = data[start + 6:name_end].decode("utf-8", errors="replace").rstrip("\x00")
        else:
            name = ""
        records.append(Record(key=key, start=start, end=end, name=name))
    return records


def build_store_header(records: list[Record], offsets: list[int]) -> bytes:
    if len(records) != len(offsets):
        raise ValueError("records and offsets differ in length")
    header = bytearray(struct.pack("<H", len(records)))
    for record, offset in zip(records, offsets):
        header += struct.pack("<HI", record.key, offset)
    return bytes(header)

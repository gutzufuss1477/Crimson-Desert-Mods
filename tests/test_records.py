from __future__ import annotations

import struct

from crimson_mod_tools.records import (
    build_store_header,
    build_u32_header,
    parse_store_header,
    parse_u32_header,
)


def _u32_record(key: int, name: str, payload: bytes) -> bytes:
    encoded = name.encode("utf-8")
    return struct.pack("<II", key, len(encoded)) + encoded + payload


def _store_record(key: int, name: str, payload: bytes) -> bytes:
    encoded = name.encode("utf-8")
    return struct.pack("<HI", key, len(encoded)) + encoded + payload


def test_u32_header_roundtrip() -> None:
    first = _u32_record(10, "Alpha", b"one")
    second = _u32_record(20, "Beta", b"two")
    data = first + second
    stub = [
        type("RecordStub", (), {"key": 10})(),
        type("RecordStub", (), {"key": 20})(),
    ]
    header = build_u32_header(stub, [0, len(first)])
    records = parse_u32_header(data, header)
    assert [(record.key, record.name, record.size) for record in records] == [
        (10, "Alpha", len(first)),
        (20, "Beta", len(second)),
    ]


def test_store_header_roundtrip() -> None:
    first = _store_record(1, "StoreA", b"a")
    second = _store_record(2, "StoreB", b"bb")
    data = first + second
    stub = [
        type("RecordStub", (), {"key": 1})(),
        type("RecordStub", (), {"key": 2})(),
    ]
    header = build_store_header(stub, [0, len(first)])
    records = parse_store_header(data, header)
    assert [(record.key, record.name, record.size) for record in records] == [
        (1, "StoreA", len(first)),
        (2, "StoreB", len(second)),
    ]

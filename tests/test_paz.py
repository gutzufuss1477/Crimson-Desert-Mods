from __future__ import annotations

from pathlib import Path

from crimson_mod_tools.paz import build_two_file_overlay, parse_pamt, pearl_abyss_checksum


def test_alden_template_and_overlay_roundtrip() -> None:
    root = Path(__file__).resolve().parents[1]
    template = (root / "assets/alden/0.pamt.template").read_bytes()
    first = b"store-data" * 10
    second = b"store-header" * 3
    pamt, paz = build_two_file_overlay(template, first, second)
    entries = parse_pamt(pamt)
    assert [entry.path for entry in entries] == [
        "gamedata/storeinfo.pabgb",
        "gamedata/storeinfo.pabgh",
    ]
    assert paz[entries[0].offset:entries[0].offset + entries[0].stored_size] == first
    assert paz[entries[1].offset:entries[1].offset + entries[1].stored_size] == second
    assert pearl_abyss_checksum(paz) != 0

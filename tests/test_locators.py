from __future__ import annotations

from crimson_mod_tools.locators import ContextLocator, locate_context


def test_context_locator_uses_surrounding_bytes() -> None:
    data = b"prefixAAAA" + b"\x11\x22\x33\x44" + b"BBBBsuffix"
    locator = ContextLocator(
        relative_offset=10,
        before=b"AAAA",
        after=b"BBBB",
        expected=b"\x11\x22\x33\x44",
    )
    assert locate_context(data, locator) == 10


def test_context_locator_falls_back_to_unique_expected_value() -> None:
    data = b"abc" + b"\x01\x02\x03\x04" + b"xyz"
    locator = ContextLocator(
        relative_offset=0,
        before=b"not-present",
        after=b"not-present",
        expected=b"\x01\x02\x03\x04",
    )
    assert locate_context(data, locator) == 3

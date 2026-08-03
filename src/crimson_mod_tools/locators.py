from __future__ import annotations

from dataclasses import dataclass

from .common import find_all


@dataclass(frozen=True)
class ContextLocator:
    relative_offset: int
    before: bytes
    after: bytes
    expected: bytes

    @classmethod
    def from_recipe(cls, recipe: dict) -> "ContextLocator":
        return cls(
            relative_offset=int(recipe["relative_offset"]),
            before=bytes.fromhex(recipe["before"]),
            after=bytes.fromhex(recipe["after"]),
            expected=bytes.fromhex(recipe["expected"]),
        )


def locate_context(record: bytes, locator: ContextLocator, *, wildcard_size: int | None = None) -> int:
    """Locate a value using stable bytes before and after it.

    The returned offset points at the wildcard/value bytes, not at the prefix.
    """
    gap = len(locator.expected) if wildcard_size is None else wildcard_size
    matches: list[int] = []
    for prefix_start in find_all(record, locator.before):
        value_start = prefix_start + len(locator.before)
        suffix_start = value_start + gap
        if suffix_start + len(locator.after) <= len(record) and record[suffix_start:suffix_start + len(locator.after)] == locator.after:
            matches.append(value_start)
    matches = sorted(set(matches))
    if len(matches) == 1:
        return matches[0]

    expected_start = locator.relative_offset
    if 0 <= expected_start <= len(record) - gap:
        before_start = expected_start - len(locator.before)
        after_start = expected_start + gap
        if (
            before_start >= 0
            and record[before_start:expected_start] == locator.before
            and record[after_start:after_start + len(locator.after)] == locator.after
        ):
            return expected_start

    exact_matches = find_all(record, locator.expected)
    if len(exact_matches) == 1:
        return exact_matches[0]

    raise RuntimeError(
        f"Context locator is not unique: context={matches}, exact={exact_matches}, baseline={locator.relative_offset}"
    )

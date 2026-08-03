from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .common import read_json


@dataclass(frozen=True)
class RecipeSet:
    root: Path
    baseline: dict[str, Any]
    mounts: dict[str, Any]
    steelheart: dict[str, Any]
    alden_prices: dict[str, Any]
    alden_store: dict[str, Any]
    healthbar: dict[str, Any]


def load_recipes(root: Path) -> RecipeSet:
    required = {
        "baseline": "baseline.json",
        "mounts": "mounts.json",
        "steelheart": "steelheart.json",
        "alden_prices": "alden_prices.json",
        "alden_store": "alden_store.json",
        "healthbar": "healthbar.json",
    }
    loaded = {name: read_json(root / filename) for name, filename in required.items()}
    return RecipeSet(root=root, **loaded)


def validate_recipe_set(recipes: RecipeSet) -> list[str]:
    errors: list[str] = []
    if recipes.baseline.get("game_version") != recipes.root.name:
        errors.append("baseline game_version must match recipe folder name")
    targets = recipes.mounts.get("target_records", [])
    if len(targets) < 300 or len(targets) != len(set(targets)):
        errors.append("mount target list is incomplete or contains duplicates")
    prices = recipes.alden_prices.get("patches", [])
    if len(prices) != 471:
        errors.append(f"expected 471 Alden price recipes, found {len(prices)}")
    entries = recipes.alden_store.get("entries", [])
    if len(entries) != 379:
        errors.append(f"expected 379 Alden store entries, found {len(entries)}")
    health_characters = recipes.healthbar.get("characters", [])
    if len(health_characters) != 3:
        errors.append("expected three Healthbar character locators")
    return errors

from __future__ import annotations

from pathlib import Path

from crimson_mod_tools.recipes import load_recipes, validate_recipe_set


def test_baseline_recipe_counts() -> None:
    root = Path(__file__).resolve().parents[1]
    recipes = load_recipes(root / "recipes/1.16.01")
    assert validate_recipe_set(recipes) == []
    assert len(recipes.mounts["target_records"]) == 380
    assert len(recipes.alden_prices["patches"]) == 471
    assert len(recipes.alden_store["entries"]) == 379
    assert len(recipes.healthbar["characters"]) == 3


def test_no_duplicate_alden_item_entry_ids() -> None:
    root = Path(__file__).resolve().parents[1]
    recipes = load_recipes(root / "recipes/1.16.01")
    item_ids = [int(entry["item_id"]) for entry in recipes.alden_store["entries"]]
    assert len(item_ids) == len(set(item_ids))

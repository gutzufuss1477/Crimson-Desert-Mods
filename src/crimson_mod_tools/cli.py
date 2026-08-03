from __future__ import annotations

import argparse
import json
import shutil
import sys
import zipfile
from dataclasses import asdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Callable

from .baseline import capture_baseline
from .builders import (
    BuildContext,
    BuildResult,
    build_alden,
    build_healthbars,
    build_mounts,
    build_steelheart,
    validate_patch_package,
)
from .common import (
    REQUIRED_GAME_FILES,
    extract_zip_safely,
    game_file_hashes,
    locate_game_root,
    reset_directory,
    sha256_file,
    write_json,
)
from .recipes import load_recipes, validate_recipe_set


def repository_root() -> Path:
    return Path(__file__).resolve().parents[2]


def newest_recipe_directory(root: Path) -> Path:
    candidates = sorted(path for path in (root / "recipes").iterdir() if path.is_dir())
    if not candidates:
        raise RuntimeError("No recipe directories found")
    return candidates[-1]


def _bundle_files(destination: Path, files: list[Path], extra_files: list[Path] | None = None) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.unlink(missing_ok=True)
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for file_path in sorted(files + (extra_files or [])):
            archive.write(file_path, file_path.name)
    with zipfile.ZipFile(destination, "r") as archive:
        bad = archive.testzip()
        if bad:
            raise RuntimeError(f"Bundle validation failed: {bad}")


def _run_builder(name: str, builder: Callable[[BuildContext], BuildResult], context: BuildContext) -> BuildResult:
    try:
        return builder(context)
    except Exception as exc:  # noqa: BLE001 - failures must be preserved in build report
        return BuildResult(name=name, status="failed", packages=[], details={}, error=f"{type(exc).__name__}: {exc}")


def build_command(args: argparse.Namespace) -> int:
    root = repository_root()
    recipe_dir = Path(args.recipe_dir).resolve() if args.recipe_dir else newest_recipe_directory(root)
    recipes = load_recipes(recipe_dir)
    recipe_errors = validate_recipe_set(recipes)
    if recipe_errors:
        raise RuntimeError("Recipe validation failed: " + "; ".join(recipe_errors))

    work_root = Path(args.work).resolve() if args.work else root / "work" / args.game_version
    if args.game_zip:
        extract_zip_safely(Path(args.game_zip).resolve(), work_root / "extracted")
        game_root = locate_game_root(work_root / "extracted")
    else:
        game_root = locate_game_root(Path(args.game_root).resolve())

    output_root = Path(args.output).resolve() if args.output else root / "dist" / args.game_version
    reset_directory(output_root)
    context = BuildContext(
        repo_root=root,
        game_root=game_root,
        game_version=args.game_version,
        output_root=output_root,
        recipes=recipes,
    )

    current_hashes = game_file_hashes(game_root)
    baseline_hashes = recipes.baseline.get("files", {})
    baseline_matches = {
        relative: current_hashes[relative]["sha256"] == baseline_hashes.get(relative, {}).get("sha256")
        for relative in REQUIRED_GAME_FILES
    }

    builders = (
        ("mounts", build_mounts),
        ("steelheart", build_steelheart),
        ("alden", build_alden),
        ("healthbars", build_healthbars),
    )
    results = [_run_builder(name, builder, context) for name, builder in builders]
    packages = [package for result in results for package in result.packages]

    validations: list[dict] = []
    validation_errors: list[str] = []
    for package in packages:
        try:
            validations.append(validate_patch_package(package, game_root))
        except Exception as exc:  # noqa: BLE001
            validation_errors.append(f"{package.name}: {type(exc).__name__}: {exc}")

    report = {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "game_version": args.game_version,
        "recipe_version": recipe_dir.name,
        "game_root": str(game_root),
        "baseline_hash_match": baseline_matches,
        "source_files": current_hashes,
        "results": [asdict(result) | {"packages": [path.name for path in result.packages]} for result in results],
        "package_validation": validations,
        "package_validation_errors": validation_errors,
    }
    report_path = output_root / "BUILD_REPORT.json"
    write_json(report_path, report)

    successful_packages = [package for package in packages if package.exists()]
    if successful_packages:
        _bundle_files(output_root / f"Crimson_Desert_{args.game_version}_All_Mod_Packages.zip", successful_packages, [report_path])
        normal_healthbars = [path for path in successful_packages if "Healthbar" in path.name and "DMM" not in path.name]
        dmm_healthbars = [path for path in successful_packages if "Healthbar" in path.name and "DMM" in path.name]
        if len(normal_healthbars) == 3:
            _bundle_files(output_root / f"Healthbar_Normal_{args.game_version}_All_3.zip", normal_healthbars, [report_path])
        if len(dmm_healthbars) == 3:
            _bundle_files(output_root / f"Healthbar_DMM_{args.game_version}_All_3.zip", dmm_healthbars, [report_path])

    checksum_lines = []
    for path in sorted(output_root.rglob("*.zip")):
        checksum_lines.append(f"{sha256_file(path)}  {path.relative_to(output_root).as_posix()}")
    (output_root / "SHA256SUMS.txt").write_text("\n".join(checksum_lines) + "\n", encoding="utf-8", newline="\n")

    failures = [result for result in results if result.status == "failed"]
    summary = {
        "output": str(output_root),
        "successful_build_groups": [result.name for result in results if result.status != "failed"],
        "failed_build_groups": [{"name": result.name, "error": result.error} for result in failures],
        "packages": [path.name for path in successful_packages],
        "validation_errors": validation_errors,
    }
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 1 if failures or validation_errors else 0


def validate_recipes_command(args: argparse.Namespace) -> int:
    root = repository_root()
    recipe_dir = Path(args.recipe_dir).resolve() if args.recipe_dir else newest_recipe_directory(root)
    recipes = load_recipes(recipe_dir)
    errors = validate_recipe_set(recipes)
    if errors:
        print(json.dumps({"recipe_dir": str(recipe_dir), "valid": False, "errors": errors}, indent=2))
        return 1
    summary = {
        "recipe_dir": str(recipe_dir),
        "valid": True,
        "mount_records": len(recipes.mounts["target_records"]),
        "alden_price_patches": len(recipes.alden_prices["patches"]),
        "alden_store_entries": len(recipes.alden_store["entries"]),
        "healthbar_characters": len(recipes.healthbar["characters"]),
    }
    print(json.dumps(summary, indent=2))
    return 0


def required_files_command(_: argparse.Namespace) -> int:
    print("\n".join(REQUIRED_GAME_FILES))
    return 0


def hashes_command(args: argparse.Namespace) -> int:
    game_root = locate_game_root(Path(args.game_root).resolve())
    print(json.dumps(game_file_hashes(game_root), indent=2))
    return 0



def capture_baseline_command(args: argparse.Namespace) -> int:
    root = repository_root()
    previous_dir = Path(args.previous_recipe_dir).resolve() if args.previous_recipe_dir else newest_recipe_directory(root)
    previous = load_recipes(previous_dir)
    if args.game_zip:
        work_root = Path(args.work).resolve() if args.work else root / "work" / f"baseline-{args.game_version}"
        extract_zip_safely(Path(args.game_zip).resolve(), work_root / "extracted")
        game_root = locate_game_root(work_root / "extracted")
    else:
        game_root = locate_game_root(Path(args.game_root).resolve())
    destination = Path(args.destination).resolve() if args.destination else root / "recipes" / args.game_version
    summary = capture_baseline(
        game_root=game_root,
        built_output=Path(args.built_output).resolve(),
        previous=previous,
        destination=destination,
        game_version=args.game_version,
        force=args.force,
    )
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 0


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="crimson-mods",
        description="Semantic build and validation tools for the Crimson Desert mod collection.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    build_parser = subparsers.add_parser("build", help="Build all supported mods from clean extracted game files")
    source_group = build_parser.add_mutually_exclusive_group(required=True)
    source_group.add_argument("--game-zip", help="ZIP containing the ten required vanilla files")
    source_group.add_argument("--game-root", help="Directory containing the extracted 0008 and 0012 folders")
    build_parser.add_argument("--game-version", required=True, help="Target game version, for example 1.17")
    build_parser.add_argument("--recipe-dir", help="Recipe directory; default is the newest recipes/* directory")
    build_parser.add_argument("--output", help="Output directory; default dist/<game-version>")
    build_parser.add_argument("--work", help="Temporary work directory")
    build_parser.set_defaults(function=build_command)

    recipes_parser = subparsers.add_parser("validate-recipes", help="Validate repository recipe counts and structure")
    recipes_parser.add_argument("--recipe-dir")
    recipes_parser.set_defaults(function=validate_recipes_command)

    files_parser = subparsers.add_parser("required-files", help="Print required clean game file paths")
    files_parser.set_defaults(function=required_files_command)

    hashes_parser = subparsers.add_parser("hashes", help="Print hashes for an extracted game-file set")
    hashes_parser.add_argument("--game-root", required=True)
    hashes_parser.set_defaults(function=hashes_command)

    capture_parser = subparsers.add_parser("capture-baseline", help="Capture a new recipe baseline after successful in-game testing")
    capture_source = capture_parser.add_mutually_exclusive_group(required=True)
    capture_source.add_argument("--game-zip")
    capture_source.add_argument("--game-root")
    capture_parser.add_argument("--game-version", required=True)
    capture_parser.add_argument("--built-output", required=True)
    capture_parser.add_argument("--previous-recipe-dir")
    capture_parser.add_argument("--destination")
    capture_parser.add_argument("--work")
    capture_parser.add_argument("--force", action="store_true")
    capture_parser.set_defaults(function=capture_baseline_command)
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = create_parser()
    args = parser.parse_args(argv)
    try:
        return int(args.function(args))
    except Exception as exc:  # noqa: BLE001
        print(f"ERROR: {type(exc).__name__}: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

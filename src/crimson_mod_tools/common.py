from __future__ import annotations

import hashlib
import json
import shutil
import zipfile
from pathlib import Path
from typing import Any, Iterable


REQUIRED_GAME_FILES = (
    "0008/gamedata/binary__/client/bin/characterinfo.pabgb",
    "0008/gamedata/binary__/client/bin/characterinfo.pabgh",
    "0008/gamedata/binary__/client/bin/iteminfo.pabgb",
    "0008/gamedata/binary__/client/bin/iteminfo.pabgh",
    "0008/gamedata/binary__/client/bin/skill.pabgb",
    "0008/gamedata/binary__/client/bin/skill.pabgh",
    "0008/gamedata/binary__/client/bin/storeinfo.pabgb",
    "0008/gamedata/binary__/client/bin/storeinfo.pabgh",
    "0012/ui/xml/gamemain/play/subtitletagview.css",
    "0012/ui/xml/gamemain/play/subtitletagview.html",
)


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(value, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
        newline="\n",
    )


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def find_all(data: bytes, needle: bytes, start: int = 0, end: int | None = None) -> list[int]:
    if not needle:
        raise ValueError("needle must not be empty")
    limit = len(data) if end is None else end
    positions: list[int] = []
    cursor = start
    while True:
        cursor = data.find(needle, cursor, limit)
        if cursor < 0:
            return positions
        positions.append(cursor)
        cursor += 1


def compact_hex(data: bytes) -> str:
    return data.hex().upper()


def spaced_hex(data: bytes) -> str:
    return " ".join(f"{byte:02x}" for byte in data)


def reset_directory(path: Path) -> None:
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True, exist_ok=True)


def zip_directory(folder: Path, destination: Path, *, include_root: bool = True) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.unlink(missing_ok=True)
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for file_path in sorted(folder.rglob("*")):
            if not file_path.is_file():
                continue
            if include_root:
                member = Path(folder.name) / file_path.relative_to(folder)
            else:
                member = file_path.relative_to(folder)
            archive.write(file_path, member.as_posix())
    with zipfile.ZipFile(destination, "r") as archive:
        bad_member = archive.testzip()
        if bad_member:
            raise RuntimeError(f"ZIP validation failed for {destination}: {bad_member}")


def locate_game_root(path: Path) -> Path:
    """Locate the directory that directly contains 0008 and 0012."""
    path = path.resolve()
    candidates = [path]
    candidates.extend(p for p in path.rglob("*") if p.is_dir())
    matches: list[Path] = []
    for candidate in candidates:
        if all((candidate / relative).is_file() for relative in REQUIRED_GAME_FILES):
            matches.append(candidate)
    if len(matches) != 1:
        raise RuntimeError(
            f"Expected exactly one extracted game root below {path}; found {len(matches)}: "
            + ", ".join(str(value) for value in matches[:5])
        )
    return matches[0]


def extract_zip_safely(zip_path: Path, destination: Path) -> None:
    reset_directory(destination)
    with zipfile.ZipFile(zip_path, "r") as archive:
        destination_resolved = destination.resolve()
        for member in archive.infolist():
            target = (destination / member.filename).resolve()
            if destination_resolved not in target.parents and target != destination_resolved:
                raise RuntimeError(f"Unsafe ZIP path: {member.filename}")
        archive.extractall(destination)


def game_file_hashes(game_root: Path) -> dict[str, dict[str, int | str]]:
    result: dict[str, dict[str, int | str]] = {}
    for relative in REQUIRED_GAME_FILES:
        path = game_root / relative
        result[relative] = {"bytes": path.stat().st_size, "sha256": sha256_file(path)}
    return result


def ensure_unique(values: Iterable[int], label: str) -> None:
    materialized = list(values)
    if len(materialized) != len(set(materialized)):
        raise RuntimeError(f"Duplicate {label} detected")

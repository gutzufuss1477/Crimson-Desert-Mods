# Windows-Einrichtung

## Voraussetzungen

- Windows 10 oder 11
- Git
- Python 3.11 oder neuer
- DMM
- Crimson Desert

## Repository klonen

`git clone https://github.com/gutzufuss1477/Crimson-Desert-Mods.git`

Danach in den Repository-Ordner wechseln.

## Entwicklungsumgebung optional einrichten

`python -m venv .venv`

`.\.venv\Scripts\Activate.ps1`

`python -m pip install -U pip`

`python -m pip install -e . -r requirements-dev.txt`

## Tests

`python -m pytest -q`

## Mod-Build

Siehe `START_HERE_DE.md` und `UPDATE_WORKFLOW_DE.md`.

# Crimson Desert Mods – Build- und Update-System

Dieses Repository verwaltet die aktuellen Crimson-Desert-Mods als reproduzierbare Patchrezepte und Buildskripte. Die Originaldateien des Spiels werden nicht in Git gespeichert.

## Verwaltete Mods

- Alden AIO Shop + All Items 1 Copper
- All Mounts LvL 5 Speed
- All Mounts LvL 5 All Stats
- Steelheart Horseshoes +20 Stamina Regen
- Healthbar Always On – Caites Multitarget
- Healthbar Always On – Classic Vanilla Single Target
- Healthbar Always On – Vanilla Multitarget
- alle drei Healthbar-Varianten zusätzlich im DMM-Format

## Funktionsprinzip

Die Builder arbeiten nicht nur mit alten absoluten Offsets. Sie suchen die relevanten Datensätze anhand von IDs, Namen, Kontextsignaturen und Strukturmerkmalen neu. Jede erzeugte JSON-Patchstelle wird gegen die aktuelle Vanilla-Datei validiert.

Falls ein Spielupdate ein Datenformat verändert, wird der betroffene Mod blockiert und im `BUILD_REPORT.json` mit der konkreten Ursache aufgeführt. Das System erstellt in diesem Fall keine scheinbar erfolgreiche, aber unzuverlässige Version.

## Ersteinrichtung

Der kürzeste Einstieg steht in [START_HERE_DE.md](START_HERE_DE.md). Die vollständige Windows-Anleitung steht in [SETUP_WINDOWS_DE.md](SETUP_WINDOWS_DE.md).

Nach dem Entpacken reicht für das erstmalige private GitHub-Repository:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\Setup-GitHub.ps1 -Owner gutzufuss1477 -Repository Crimson-Desert-Mods -Visibility private -CreateInitialRelease -OpenInBrowser
```

## Update nach einem neuen Spielpatch

Die zehn sauberen Vanilla-Dateien wie bisher in eine ZIP-Datei packen. Danach:

```powershell
.\scripts\Update-Mods.ps1 -GameVersion 1.17 -GameZip "C:\Modding\Crimson_Desert_1.17_Vanilla.zip" -OpenOutput
```

Das Ergebnis liegt in `dist\1.17`.

## Nach dem Ingame-Test

Erst wenn alle erzeugten Mods im Spiel bestätigt wurden, darf die neue Version als Ausgangsbasis gespeichert werden:

```powershell
.\scripts\Promote-Baseline.ps1 -GameVersion 1.17 -GameZip "C:\Modding\Crimson_Desert_1.17_Vanilla.zip" -ConfirmedInGame -CommitAndPush
```

Danach kann die getestete Version veröffentlicht werden:

```powershell
.\scripts\Publish-Release.ps1 -Owner gutzufuss1477 -Repository Crimson-Desert-Mods -Version 1.17
```

## Repository-Inhalte

- `src/crimson_mod_tools/` – Builder, Parser und Validierung
- `recipes/` – getestete semantische Patchrezepte
- `assets/alden/` – minimale selbst erzeugte PAZ/PAMT-Paketvorlage
- `scripts/` – Windows-Automatisierung
- `tests/` – Tests ohne Original-Spieldateien
- `.github/workflows/` – GitHub-Actions-Prüfung des Quellcodes und der Rezepte

## Nicht in Git enthalten

- Vanilla-Spieldateien
- extrahierte Spielarchive
- gebaute Mod-ZIPs
- temporäre Dateien
- lokale Python-Umgebung

Diese Daten werden durch `.gitignore` ausgeschlossen. Die fertigen Pakete können lokal als privater GitHub-Release hochgeladen oder wie bisher auf Nexus Mods veröffentlicht werden.

## Wichtige Installationskonflikte

- Genau eine Healthbar-Variante aktivieren.
- Mount Speed und Mount All Stats nicht gleichzeitig aktivieren.
- Vor einem Versionswechsel alte Modpakete entfernen beziehungsweise im Mod Manager auf Vanilla zurücksetzen.

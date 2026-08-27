# Update-Workflow nach einem Crimson-Desert-Patch

Das Repository ist ab 2.00.00 vollständig auf DMM ausgerichtet. JMM-Ausgaben werden nicht mehr gebaut.

## Benötigte saubere Vanilla-Dateien

Für den vollständigen Build werden diese 12 Dateien benötigt:

- `0008/gamedata/binary__/client/bin/characterinfo.pabgb`
- `0008/gamedata/binary__/client/bin/characterinfo.pabgh`
- `0008/gamedata/binary__/client/bin/iteminfo.pabgb`
- `0008/gamedata/binary__/client/bin/iteminfo.pabgh`
- `0008/gamedata/binary__/client/bin/skill.pabgb`
- `0008/gamedata/binary__/client/bin/skill.pabgh`
- `0008/gamedata/binary__/client/bin/storeinfo.pabgb`
- `0008/gamedata/binary__/client/bin/storeinfo.pabgh`
- `0008/gamedata/binary__/client/bin/buffinfo.pabgb`
- `0008/gamedata/binary__/client/bin/buffinfo.pabgh`
- `0012/ui/xml/gamemain/play/subtitletagview.css`
- `0012/ui/xml/gamemain/play/subtitletagview.html`

Vor dem Extrahieren in DMM auf Vanilla zurücksetzen und die Spieldateien über Steam prüfen.

## Build

Die Dateien mit Ordnerstruktur in eine ZIP packen. Danach:

`powershell -ExecutionPolicy Bypass -File .\scripts\Build-DMM-Mods.ps1 -GameZip "C:\Modding\Crimson_Desert_Vanilla.zip"`

Der Builder bricht ab, wenn eine der bestätigten Strukturen nicht mehr eindeutig gefunden wird. Dadurch werden bei einem Spielupdate keine alten Offsets blind weiterverwendet.

## Nach einem neuen Spielupdate

1. Neue Vanilla-Dateien sichern.
2. Versionsnummer im Builder und in den Rezepten auf die neue Spielversion anheben.
3. Builder ausführen.
4. Jeden Mod einzeln im Spiel testen.
5. Alden, Steelheart, eine Mount-Version und eine Healthbar-Version kombiniert testen.
6. Erst nach erfolgreichem Ingame-Test die neuen Pakete unter `release-assets/<Version>/` übernehmen.
7. README und Changelog aktualisieren.
8. Commit und Push durchführen.

## Spezielle Baselines

- Alden verwendet die ursprüngliche 379-Item-Liste aus `recipes/2.00.00/alden_catalog.tsv`.
- Steelheart wird semantisch über ItemInfo gepatcht, nicht als roher Bytepatch.
- Healthbar ersetzt SkillInfo nur für Skill 1201 und verwendet drei gezielte CharacterInfo-Patches.
- Mount Speed ist eine echte Teilmenge von Mount All Stats und ändert ausschliesslich den Speed-Wert.

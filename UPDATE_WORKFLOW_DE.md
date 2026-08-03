# Ablauf bei einem neuen Crimson-Desert-Update

## Phase A – saubere Vanilla-Dateien beschaffen

1. Alle Mods deaktivieren.
2. Im verwendeten Mod Manager auf Vanilla zurücksetzen.
3. Spieldateien über Steam prüfen.
4. Spielversion kontrollieren.
5. Die folgenden Dateien in eine ZIP packen:

```text
0008/gamedata/binary__/client/bin/characterinfo.pabgb
0008/gamedata/binary__/client/bin/characterinfo.pabgh
0008/gamedata/binary__/client/bin/iteminfo.pabgb
0008/gamedata/binary__/client/bin/iteminfo.pabgh
0008/gamedata/binary__/client/bin/skill.pabgb
0008/gamedata/binary__/client/bin/skill.pabgh
0008/gamedata/binary__/client/bin/storeinfo.pabgb
0008/gamedata/binary__/client/bin/storeinfo.pabgh
0012/ui/xml/gamemain/play/subtitletagview.css
0012/ui/xml/gamemain/play/subtitletagview.html
```

Ein zusätzlicher oberster Ordner wie `extract/` ist erlaubt.

## Phase B – automatische Aktualisierung

Beispiel für Version 1.17:

```powershell
.\scripts\Update-Mods.ps1 `
  -GameVersion 1.17 `
  -GameZip "C:\Modding\Crimson_Desert_1.17_Vanilla.zip" `
  -OpenOutput
```

Der Builder erzeugt:

- sieben normale Modpakete
- drei DMM-Healthbar-Pakete
- Healthbar-Sammelpakete
- ein Gesamtpaket
- `BUILD_REPORT.json`
- `SHA256SUMS.txt`

## Phase C – Bericht auswerten

`successful_build_groups` muss diese vier Gruppen enthalten:

```text
mounts
steelheart
alden
healthbars
```

`failed_build_groups` und `validation_errors` müssen leer sein.

Warnungen bei geänderten Mount-Datensatzmengen sind nicht automatisch ein Fehler, müssen aber geprüft werden.

## Phase D – Ingame-Test

Mindestens folgende Tests durchführen:

1. Mount Speed und All Stats getrennt testen.
2. Steelheart-Ausdauerregeneration prüfen.
3. Alden öffnen und Anzahl, Preise und Mengen prüfen.
4. Jede normale Healthbar-Variante einzeln testen.
5. Jede DMM-Healthbar-Variante einzeln mounten und testen.
6. Spielstart und Gebietswechsel auf Abstürze prüfen.

## Phase E – neue Basis übernehmen

Nur nach erfolgreichem Ingame-Test:

```powershell
.\scripts\Promote-Baseline.ps1 `
  -GameVersion 1.17 `
  -GameZip "C:\Modding\Crimson_Desert_1.17_Vanilla.zip" `
  -ConfirmedInGame `
  -CommitAndPush
```

Dadurch werden neue Kontextsignaturen und Dateihashes unter `recipes\1.17` gespeichert. Die Original-Spieldateien werden nicht übernommen.

## Phase F – Release veröffentlichen

```powershell
.\scripts\Publish-Release.ps1 `
  -Owner gutzufuss1477 `
  -Repository Crimson-Desert-Mods `
  -Version 1.17
```

Das Skript erstellt den Tag `game-1.17`. Falls der Release bereits existiert, werden die Assets ersetzt und die Release-Informationen aktualisiert.

## Fehlerfall

Wenn eine Buildgruppe blockiert wird:

- keine ZIP dieser Gruppe veröffentlichen
- `BUILD_REPORT.json` aufbewahren
- aktuelle Vanilla-ZIP und die letzte bestätigte Version für die Analyse bereithalten
- nicht einfach alte Offsets übernehmen

Die anderen erfolgreich gebauten Gruppen bleiben im Ausgabeordner erhalten.

# Crimson Desert Mods

DMM-only Build- und Release-Repository für die Crimson-Desert-Mods von Blablup.

## Aktueller Stand

- Spielversion: **Crimson Desert 2.00.00**
- Mod Manager: **DMM**
- Mit DMM 1.9.3 getestet
- **JMM wird nicht mehr unterstützt oder gepflegt.**
- Die Kernimplementierungen von Alden, Steelheart, Mount All Stats und Healthbar Vanilla Multitarget wurden im Spiel bestätigt. Speed ist die exakt validierte Speed-Teilmenge der All-Stats-Version; die beiden weiteren Healthbar-Varianten verwenden dieselbe bestätigte Skill-/CharacterInfo-Basis.

## Enthaltene Mods

| Mod | Status 2.00.00 | Hinweise |
|---|---|---|
| Alden AIO Shop + All Items 1 Copper | im Spiel bestätigt | Exakter alter 379-Item-Katalog, alte Reihenfolge und alte Bestände, Preise 1 Copper |
| All Mounts LvL 5 Speed | statisch bestätigt | Nur Speed auf Level 5 |
| All Mounts LvL 5 All Stats | im Spiel bestätigt | Speed, Acceleration, Turning und Jump auf Level 5 |
| Steelheart Horseshoes +20 Stamina Regen | im Spiel bestätigt | Semantischer DMM-ItemInfo-Patch nur für Item 1000594 |
| Healthbar Always On - Caites Multitarget | gleiche bestätigte Basis | DMM-Dateiersatz + 3 gezielte CharacterInfo-Patches |
| Healthbar Always On - Classic Vanilla Single Target | gleiche bestätigte Basis | DMM-Dateiersatz + 3 gezielte CharacterInfo-Patches |
| Healthbar Always On - Vanilla Multitarget | im Spiel bestätigt | DMM-Dateiersatz + 3 gezielte CharacterInfo-Patches |

## Fertige Downloads

Die bestätigten Pakete liegen unter [`release-assets/2.00.00/`](release-assets/2.00.00/).

Die Dateinamen und internen Modnamen folgen nur noch diesem Schema:

`Mod_Name_2.00.00`

Es gibt keine `DMM`, `RC`, `R2`, `R5` oder `TEST`-Zusätze mehr in den finalen Modnamen.

## Wichtige Konflikte

- Nur **eine** der beiden Mount-Versionen gleichzeitig aktivieren.
- Nur **eine** der drei Healthbar-Versionen gleichzeitig aktivieren.
- Vor einem Versionswechsel alte Modpakete in DMM entfernen und auf Vanilla zurücksetzen.

## Build-System

Die finalen Mods können aus sauberen Vanilla-Dateien reproduziert werden:

- `src/crimson_mod_tools/dmm_builder.py` - Builder und Validierung
- `recipes/2.00.00/` - bestätigte Rezepte und Baselines
- `scripts/Build-DMM-Mods.ps1` - Windows-Build
- `tests/` - Struktur- und Release-Tests
- `release-assets/2.00.00/` - im Spiel bestätigte finale Mod-ZIPs

Originaldateien des Spiels werden nicht in Git gespeichert.

## Changelog 2.00.00

Für alle Mods gilt:

> Updated for Crimson Desert 2.00.00.  
> The mod has been rebuilt specifically for DMM and is no longer maintained for JMM.  
> Internal patches and game data have been updated to match the current game version.

Weitere Details: [`CHANGELOG.md`](CHANGELOG.md)

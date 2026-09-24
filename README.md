# Crimson Desert Mods

Build- und Release-Repository für die Crimson-Desert-Mods von Blablup. Die Pakete können über DMM installiert werden; ASI-Plugins lassen sich zusätzlich manuell mit einem ASI-Loader verwenden.

## Aktueller Stand

- Aktuelle Mount-Mods: **Crimson Desert 2.03.00**
- Mining Helmet Always On: **Crimson Desert 2.03.00**
- Die übrigen vorhandenen Mods bleiben auf ihrem zuletzt bestätigten Stand **2.00.00**, bis sie separat aktualisiert werden.
- Mod Manager: **DMM**
- JMM wird nicht mehr unterstützt oder gepflegt.
- Die 2.03.00-Mount-Mods werden als DMM-verwaltete ASI-Plugins ausgeliefert, weil die bisherigen statischen CharacterInfo-Patches die aktiven Werte bestehender Level-5-Mounts in 2.03.00 nicht mehr zuverlässig steuern.

## Enthaltene Mods

| Mod | Aktuelle Version | Status | Hinweise |
| --- | --- | --- | --- |
| Mining Helmet Always On | **2.03.00** | im Spiel bestätigt | Automatisches blaues Ressourcenleuchten mit beliebiger Kopfbedeckung; DMM oder manuelle ASI-Installation |
| Alden AIO Shop + All Items 1 Copper | 2.00.00 | im Spiel bestätigt | Exakter alter 379-Item-Katalog, alte Reihenfolge und alte Bestände, Preise 1 Copper |
| All Mounts LvL 5 Speed | **2.03.00** | aktualisiert | Nur Movement Speed auf Level 5; DMM-managed ASI |
| All Mounts LvL 5 All Stats | **2.03.00** | im Spiel bestätigt | Speed, Acceleration, Turning und Jump auf Level 5; DMM-managed ASI |
| Steelheart Horseshoes +20 Stamina Regen | 2.00.00 | im Spiel bestätigt | Semantischer DMM-ItemInfo-Patch nur für Item 1000594 |
| Healthbar Always On - Caites Multitarget | 2.00.00 | gleiche bestätigte Basis | DMM-Dateiersatz + CharacterInfo-Patches |
| Healthbar Always On - Classic Vanilla Single Target | 2.00.00 | gleiche bestätigte Basis | DMM-Dateiersatz + CharacterInfo-Patches |
| Healthbar Always On - Vanilla Multitarget | 2.00.00 | im Spiel bestätigt | DMM-Dateiersatz + CharacterInfo-Patches |

## Fertige Downloads

- `release-assets/2.03.00/` – aktuelle Mount-Mods und Mining Helmet Always On für Crimson Desert 2.03.00
- `release-assets/2.00.00/` – bisherige 2.00.00-Releases

Die Dateinamen und internen Modnamen folgen weiterhin diesem Schema:

`Mod_Name_GameVersion`

Es gibt keine `DMM`, `RC`, `R2`, `R5` oder `TEST`-Zusätze in finalen Modnamen.

## Wichtige Konflikte

- Nur **eine** der beiden Mount-Versionen gleichzeitig aktivieren.
- Nur eine der drei Healthbar-Versionen gleichzeitig aktivieren.
- Vor einem Versionswechsel alte Modpakete in DMM entfernen und auf Vanilla zurücksetzen.

## Mount-Mods 2.03.00

### Warum wurde das Mod-Format geändert?

Crimson Desert 2.03.00 hat die Verarbeitung der aktiven Mount-Stats geändert.

Die bisherigen Mount-Mods änderten die Level-5-Werte statisch in CharacterInfo. Diese Daten lassen sich auch in 2.03.00 weiterhin korrekt patchen, werden bei bereits vorhandenen Level-5-Mounts aber nicht mehr zuverlässig als aktive Werte übernommen.

Die 2.03.00-Versionen verwenden deshalb den im Spiel verifizierten aktuellen Runtime-Mount-Stat-Pfad und werden als **DMM-managed ASI plugins** ausgeliefert.

Für den Benutzer bleibt die Installation gleich: Release-ZIP in DMM importieren, aktivieren und anwenden. Cheat Engine oder manuelle Runtime-Tools werden nicht benötigt.

### Varianten

- **All Mounts LvL 5 All Stats** – setzt Movement Speed, Acceleration, Turning/Handling und Jump auf 5.
- **All Mounts LvL 5 Speed** – setzt ausschliesslich Movement Speed auf 5.

## Build-System

Die bisherigen statischen DMM-Datenmods können weiterhin über das vorhandene Build-System reproduziert werden:

- `src/crimson_mod_tools/dmm_builder.py`
- `recipes/2.00.00/`
- `scripts/Build-DMM-Mods.ps1`
- `tests/`

Die Quellcodes der neuen 2.03.00-Mount-Plugins liegen direkt unter:

- `mods/all-mounts-level-5-all-stats/src/`
- `mods/all-mounts-level-5-speed/src/`
- `mods/mining-helmet-always-on/src/`

Originaldateien des Spiels werden nicht in Git gespeichert.

## Changelog 2.03.00

> Added **Mining Helmet Always On 1.0.0** for Crimson Desert 2.03.00.<br>
> The native blue mining-resource highlight now starts automatically without requiring the Mining Helmet.<br>
> The green full-screen filter remains disabled and the original Mining Helmet behavior is preserved.<br>
> The mod uses the game's normal visibility and fade distance.

> Updated the mount mods for Crimson Desert 2.03.00.  
> Reworked the mount stat modification system for the current game version.  
> The mount mods now use DMM-managed ASI plugins because the previous static CharacterInfo patches no longer reliably control the active stats of existing mounts.  
> No Cheat Engine or manual runtime setup is required.

Weitere Details: `CHANGELOG.md`

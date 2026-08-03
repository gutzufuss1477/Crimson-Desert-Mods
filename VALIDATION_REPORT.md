# Lokaler Validierungsbericht

Das Repository wurde vor der Übergabe mit den bestätigten Vanilla-Dateien von Crimson Desert 1.16.01 geprüft.

## Erfolgreiche Prüfungen

- Python-Quellcode vollständig kompiliert.
- Sieben automatisierte Tests erfolgreich.
- Rezeptstruktur erfolgreich validiert.
- 380 Mount-Datensätze erfasst.
- 471 Alden-Preisrezepte erfasst.
- 379 Alden-Shopangebote erfasst.
- Drei Healthbar-Charakterpatches erfasst.
- Vollständiger Build direkt aus der Vanilla-ZIP erfolgreich.
- Vier Buildgruppen erfolgreich: Mounts, Steelheart, Alden und Healthbars.
- Zehn einzelne Modpakete erzeugt und validiert.
- Drei zusätzliche Sammelarchive erzeugt und auf ZIP-Integrität geprüft.
- Neue Baseline-Erfassung testweise erfolgreich ausgeführt.
- Git-Staging geprüft: Vanilla-Dateien, Release-ZIPs und lokale Buildausgaben werden nicht committed.

## Absichtliche Grenze

GitHub Actions kann ohne proprietäre Vanilla-Dateien nur Quellcode, Tests und Rezeptstruktur prüfen. Der eigentliche Mod-Build läuft deshalb lokal mit den vom Benutzer extrahierten Spieldateien. Fehlerhafte oder nicht eindeutig lokalisierbare Patchstellen blockieren die jeweilige Buildgruppe und werden im `BUILD_REPORT.json` dokumentiert.

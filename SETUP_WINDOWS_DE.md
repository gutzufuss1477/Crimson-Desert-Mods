# Windows- und GitHub-Einrichtung

## 1. Paket entpacken

Das Repository-Paket beispielsweise hier entpacken:

```text
C:\Modding\Crimson-Desert-Mods
```

Der Ordner muss direkt `README.md`, `scripts`, `src` und `recipes` enthalten. Nicht nur einen Unterordner daraus kopieren.

## 2. Benötigte Programme installieren

PowerShell als normaler Benutzer im entpackten Repository-Ordner öffnen und ausführen:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\Install-Prerequisites.ps1
```

Das Skript verwendet WinGet mit den eindeutigen Paket-IDs `Git.Git`, `GitHub.cli` und `Python.Python.3.13`. Alternativ können die drei Programme manuell installiert werden.

Danach PowerShell vollständig schliessen und neu öffnen.

Prüfung:

```powershell
git --version
gh --version
py -3 --version
```

## 3. In den Repository-Ordner wechseln

```powershell
cd C:\Modding\Crimson-Desert-Mods
Set-ExecutionPolicy -Scope Process Bypass
```

`-Scope Process` ändert die Richtlinie nur für dieses PowerShell-Fenster.

## 4. GitHub anmelden und Repository erstellen

```powershell
.\scripts\Setup-GitHub.ps1 `
  -Owner gutzufuss1477 `
  -Repository Crimson-Desert-Mods `
  -Visibility private `
  -CreateInitialRelease `
  -OpenInBrowser
```

Das Skript führt folgende Schritte aus:

1. GitHub-Login im Browser, falls noch nicht angemeldet.
2. Lokales Git-Repository initialisieren.
3. Quellcode, Rezepte und Dokumentation committen.
4. Das private Repository `gutzufuss1477/Crimson-Desert-Mods` erstellen.
5. Den Branch `main` pushen.
6. Die bestätigten 1.16.01-Pakete als privaten Release `game-1.16.01` hochladen.

Es werden keine GitHub-Passwörter oder Personal Access Tokens in Dateien gespeichert.

## 5. Installation prüfen

```powershell
.\scripts\Verify-Environment.ps1
```

Danach auf GitHub kontrollieren:

- Repository ist privat.
- Branch `main` ist vorhanden.
- Unter **Actions** ist der Workflow `Validate repository` erfolgreich.
- Unter **Releases** ist `Crimson Desert Mods 1.16.01` vorhanden, falls `-CreateInitialRelease` verwendet wurde.

## 6. Repository existiert bereits

Das Setup-Skript erkennt ein vorhandenes Repository. Es erstellt dann kein zweites, sondern verbindet den lokalen Ordner mit dem bestehenden Repository und pusht `main`.

Bei einem absichtlich anderen Namen:

```powershell
.\scripts\Setup-GitHub.ps1 -Owner gutzufuss1477 -Repository Mein-Anderer-Name -Visibility private
```

## 7. Öffentliches Repository

Der Quellcode und die Patchrezepte können grundsätzlich öffentlich verwaltet werden. Die DMM-Healthbar-Pakete enthalten jedoch vollständig neu aufgebaute, spielabgeleitete Binärdateien. Deshalb sollte das Repository zunächst privat bleiben. Vor einer öffentlichen Veröffentlichung müssen die Rechte und Plattformregeln separat geprüft werden.

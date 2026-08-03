# Hier beginnen

## Einmalige Einrichtung

1. Dieses Paket nach `C:\Modding\Crimson-Desert-Mods` entpacken.
2. PowerShell im entpackten Ordner öffnen.
3. Nur für dieses Fenster Skriptausführung erlauben:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
```

4. Git, GitHub CLI und Python installieren:

```powershell
.\scripts\Install-Prerequisites.ps1
```

5. PowerShell vollständig schliessen und neu im Repository-Ordner öffnen.
6. Das private GitHub-Repository erstellen und den bestätigten Release 1.16.01 hochladen:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\Setup-GitHub.ps1 -Owner gutzufuss1477 -Repository Crimson-Desert-Mods -Visibility private -CreateInitialRelease -OpenInBrowser
```

Beim ersten Lauf öffnet GitHub CLI die GitHub-Anmeldung im Browser. Keine Tokens oder Passwörter in diesem Ordner speichern.

## Beim nächsten Spielupdate

Die sauberen Vanilla-Dateien in eine ZIP packen und beispielsweise für Version 1.17 ausführen:

```powershell
.\scripts\Update-Mods.ps1 -GameVersion 1.17 -GameZip "C:\Modding\Crimson_Desert_1.17_Vanilla.zip" -OpenOutput
```

Danach alle Pakete im Spiel testen. Erst nach erfolgreichem Test die neue Basis übernehmen und pushen:

```powershell
.\scripts\Promote-Baseline.ps1 -GameVersion 1.17 -GameZip "C:\Modding\Crimson_Desert_1.17_Vanilla.zip" -ConfirmedInGame -CommitAndPush
```

Anschliessend den getesteten Release veröffentlichen:

```powershell
.\scripts\Publish-Release.ps1 -Owner gutzufuss1477 -Repository Crimson-Desert-Mods -Version 1.17
```

Die ausführlichen Erklärungen stehen in `SETUP_WINDOWS_DE.md` und `UPDATE_WORKFLOW_DE.md`.

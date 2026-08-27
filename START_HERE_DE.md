# Schnellstart

## Fertige Mods verwenden

Die bestätigten Mod-ZIPs liegen in:

`release-assets\2.00.00\`

Die gewünschte ZIP direkt in DMM importieren, aktivieren und einhängen.

Wichtig:

- Nur eine Mount-Version gleichzeitig aktivieren.
- Nur eine Healthbar-Version gleichzeitig aktivieren.
- JMM wird nicht mehr unterstützt.

## Mods neu bauen

Für einen reproduzierbaren Build werden die 12 sauberen Vanilla-Dateien benötigt, die in `UPDATE_WORKFLOW_DE.md` aufgeführt sind.

Danach unter Windows:

`powershell -ExecutionPolicy Bypass -File .\scripts\Build-DMM-Mods.ps1 -GameZip "C:\Pfad\Crimson_Desert_Vanilla.zip"`

Die neu erzeugten Pakete liegen danach unter:

`dist\2.00.00\packages\`

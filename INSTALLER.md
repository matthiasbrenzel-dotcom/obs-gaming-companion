# Windows-Installer

## Ziel
Nach einem erfolgreichen Windows-Build liegt das Plugin im OBS-kompatiblen Staging-Layout:

```text
release/windows-x64/
├─ obs-plugins/64bit/obs-gaming-companion.dll
└─ data/obs-plugins/obs-gaming-companion/...
```

## Lokale Installation

```powershell
.\scripts\install-local.ps1
```

Der Installer sucht standardmäßig `C:\Program Files\obs-studio`. Ein anderer Pfad kann mit `-ObsPath` angegeben werden.

## Setup.exe bauen
1. Inno Setup 6 installieren.
2. Plugin erfolgreich bauen/stagen.
3. Ausführen:

```powershell
.\scripts\package-installer.ps1
```

Ergebnis:

```text
dist/OBS-Gaming-Companion-0.5.1-Windows-x64-Setup.exe
```

## Hinweis
Das Setup selbst ist vorbereitet, aber eine echte Windows-DLL muss in einer Windows-Build-Umgebung gegen die OBS/libobs- und Qt-Abhängigkeiten kompiliert werden. Das kann nicht durch bloßes Packen des Quellcodes ersetzt werden.

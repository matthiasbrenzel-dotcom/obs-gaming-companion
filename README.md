# OBS Gaming Companion 0.5.1

Gaming-Erweiterung für OBS Studio 32.2.x mit eigenem Dock für Replays, 16:9-Clips und 9:16-Shorts.

## Funktionen
- ein Hotkey speichert dasselbe Highlight als **16:9-Clip + 9:16-Short**
- Short-Länge: **30 / 60 / 90 Sekunden**
- 1080×1920-Short mit weichgezeichnetem 9:16-Hintergrund
- optionales Logo-Overlay
- optionale Webcam-Hervorhebung aus einer Ecke des 16:9-Replays
- automatische Spiel-/Programm-Erkennung unter Windows
- Highlight-Marker: Kill, Highlight, Lustig, Bug
- Marker-Protokoll im JSONL-Format
- OBS-Dock + OBS-Hotkeys
- persistente Einstellungen
- **Windows-Installer-Struktur** mit PowerShell-Installer und Inno Setup

## Ausgabe
```text
OBS Gaming Companion/
├─ Clips/
├─ Shorts/
└─ Markers/
```

## Voraussetzungen zur Laufzeit
- OBS Studio 32.2.x (Windows x64)
- FFmpeg als `ffmpeg.exe` im PATH oder im Dock ausgewählt

## Voraussetzungen zum Bauen
- Windows 10/11 x64
- Visual Studio 2022 C++ Toolchain
- CMake 3.28+
- OBS/libobs + obs-frontend-api Development-CMake-Pakete
- Qt 6 Core + Widgets passend zur OBS-Build-Umgebung
- optional Inno Setup 6 für die Setup.exe

Das offizielle OBS-Projekt empfiehlt für neue Plugins das `obsproject/obs-plugintemplate`; dessen aktuelle Windows-Umgebung verwendet Visual Studio 2022 und aktuelle CMake-Presets.

## Build
```powershell
$env:OBS_DEPS_PATH='C:\Pfad\zur\OBS-CMake-Umgebung'
.\build-windows.ps1
```

## Direkt in OBS installieren
```powershell
.\scripts\install-local.ps1
```

## Setup.exe erzeugen
```powershell
.\scripts\package-installer.ps1
```

Weitere Details: `INSTALLER.md`.

## Bedienung
1. In OBS den Replay-Puffer auf mindestens die gewünschte Clip-Länge konfigurieren.
2. Replay-Puffer starten.
3. Companion-Dock öffnen.
4. 30/60/90 Sekunden wählen.
5. FFmpeg und Ausgabeordner festlegen.
6. optional Logo/Webcam aktivieren.
7. Hotkey **OBS Gaming Companion: SHORT + CLIP speichern** drücken.

## Noch nicht als „echte KI“ umgesetzt
Eine generische Kill-Erkennung über beliebige Spiele und automatische Speech-to-Text-Untertitel sind bewusst noch nicht als Schein-Funktion eingebaut. Dafür braucht es entweder spielspezifische Bild-/OCR-Erkennung bzw. ein lokales Speech-to-Text-Modell. Die vorhandenen Marker bilden die sichere Grundlage für diese nächste Stufe.


## Neu in 0.5.1 – lokale KI-Untertitel

Optional kann der fertige 9:16-Short komplett lokal transkribiert werden. Dazu werden `whisper-cli` aus whisper.cpp und ein lokales GGML-Modell ausgewählt. Der Companion extrahiert 16-kHz-Mono-Audio, lässt whisper.cpp eine SRT-Datei erzeugen und brennt diese anschließend per FFmpeg in eine zweite `*-subtitles.mp4` ein. Ohne Whisper bleibt der normale Short-Workflow unverändert.

## Auto-Highlight / Kill-Erkennung

Die generische Erkennung schreibt bei auffälligen Audio-Wechseln einen `AutoHighlightCandidate` in dieselbe JSONL-Markerdatei wie manuelle Marker. Das ist absichtlich **kein behaupteter Kill-Detektor**: zuverlässige Kill-Erkennung benötigt pro Spiel eine eigene Datenquelle (Game-API, Logdatei, Telemetrie oder einen visuellen Detector). Die Markerstruktur ist vorbereitet, damit solche Detektoren später ergänzt werden können.

### Zusätzliche Abhängigkeiten

- FFmpeg mit `subtitles`/libass-Filter
- optional: whisper.cpp `whisper-cli.exe`
- optional: ein whisper.cpp GGML-Modell, z. B. `ggml-small.bin`

Alle Transkriptionsschritte laufen lokal; das Plugin lädt keine Audioaufnahme zu einem Cloud-Dienst hoch.

## GitHub Actions (0.5.1)

Der Ordner `.github/workflows/windows-build.yml` enthält den Windows-CI-Build. Nach dem Upload zu GitHub kann der Workflow manuell über **Actions** gestartet werden. Erfolgreiche Läufe liefern eine Setup-EXE und ein portables ZIP als GitHub-Artefakte. Siehe `GITHUB-ACTIONS.md`.

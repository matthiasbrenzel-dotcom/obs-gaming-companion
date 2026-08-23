# Changelog

## 0.5.1

- GitHub Actions auf `windows-2022` festgelegt.
- Build nutzt jetzt die offiziellen Build-Skripte aus `obsproject/obs-plugintemplate`.
- Visual-Studio-Erkennung wird vor dem Build geprüft.
- OBS/libobs/Qt-Abhängigkeiten werden über die offizielle Template-Bootstrap-Logik bezogen.
- Setup-EXE und portable ZIP werden vor dem Upload verifiziert.
- Zusätzliche Austauschdatei `windows-build-0.5.1.yml` für den GitHub-Webeditor.

## 0.5.1
- GitHub Actions Workflow für Windows x64 ergänzt.
- Automatischer Build der Plugin-DLL auf `windows-2022`.
- Automatischer Inno-Setup-Build der Windows-Setup-EXE.
- Portables Windows-ZIP als zweites Artefakt.
- Manueller Start per `workflow_dispatch`.
- Automatische Release-Anhänge bei `v*`-Tags.
- Harte Prüfungen, damit ein fehlgeschlagener DLL-/Installer-Build nicht als Erfolg erscheint.

## 0.4.0
- Lokale Whisper-Untertitel und Auto-Highlight-Grundlage.
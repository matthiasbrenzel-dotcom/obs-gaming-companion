# GitHub Actions – Windows Setup bauen

1. Den gesamten Inhalt dieses Ordners in ein GitHub-Repository hochladen.
2. In GitHub **Actions → Windows Build + Setup EXE → Run workflow** öffnen.
3. Nach dem Build unter **Artifacts** `OBS-Gaming-Companion-0.5.1-Windows-x64` herunterladen.
4. Darin liegen die Setup-EXE und ein portables ZIP.
5. Für einen Release einen Tag wie `v0.5.1` pushen. Der Workflow hängt EXE und ZIP automatisch an den GitHub Release.

Hinweis: Der Workflow baut OBS-Bibliotheken auf dem Windows-Runner und kompiliert das Plugin dagegen. Falls OBS seine Dependency-/CMake-Struktur ändert, kann die CI-Anpassung nötig werden. Der Workflow bricht bei fehlender DLL oder Setup-EXE absichtlich ab.

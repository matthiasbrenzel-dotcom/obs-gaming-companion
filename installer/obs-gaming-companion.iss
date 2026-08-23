#define MyAppName "OBS Gaming Companion"
#define MyAppVersion "0.5.1"
#define MyAppPublisher "Gefechtszone"
#define MyAppExeName "obs-gaming-companion.dll"

[Setup]
AppId={{E15EDE0E-8DD3-4D62-B433-7749659D2B16}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\obs-studio
DisableDirPage=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\dist
OutputBaseFilename=OBS-Gaming-Companion-0.5.1-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#MyAppName}
SetupLogging=yes

[Files]
Source: "..\release\windows-x64\obs-plugins\64bit\obs-gaming-companion.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion
Source: "..\release\windows-x64\data\obs-plugins\obs-gaming-companion\*"; DestDir: "{app}\data\obs-plugins\obs-gaming-companion"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

[Code]
function IsObsInstallDir(Path: string): Boolean;
begin
  Result := FileExists(AddBackslash(Path) + 'bin\64bit\obs64.exe');
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID = wpSelectDir then
  begin
    if not IsObsInstallDir(WizardDirValue) then
    begin
      MsgBox('Im gewählten Ordner wurde OBS Studio (bin\64bit\obs64.exe) nicht gefunden. Bitte den OBS-Studio-Installationsordner auswählen.', mbError, MB_OK);
      Result := False;
    end;
  end;
end;

function InitializeSetup(): Boolean;
begin
  Result := True;
end;

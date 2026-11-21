; NATRON WINDOWS INSTALLER
; Bump the pkgrel of PKGBUILD with every change of this file.

; The architecture of the Natron build that this Setup.exe installs: x64 (MSYS2 UCRT64), or arm64
; (MSYS2 CLANGARM64, #1494). The PKGBUILD passes /DNatronArch=arm64 for CLANGARM64.
#ifndef NatronArch
  #define NatronArch "x64"
#endif
#if NatronArch != "x64" && NatronArch != "arm64"
  #error NatronArch must be x64 or arm64
#endif
#if NatronArch == "arm64"
; The ARM64 Natron is a separate application, installed side by side with an x64 one: its own AppId,
; name, folder, Start menu group, desktop icon and file-type ProgID. The x64 Setup.exe installs in
; 32-bit mode (its uninstall key is in the 32-bit registry view) while the ARM64 one installs in
; 64-bit mode, so with a shared AppId neither would find the other's installation: both would go to
; Program Files\Natron, with two uninstall entries for the same files. The x64 values are unchanged.
  #define MyAppId "{{7681096C-D482-4BD1-ADD2-CA824D32EEE2}"
  #define MyAppName "Natron (ARM64)"
#else
  #define MyAppId "{{B1D54C04-B15B-4015-831A-CA5DACA60BD0}"
  #define MyAppName "Natron"
#endif
#define MyAppPublisher "The Natron developers"
#define MyAppURL "https://natrongithub.github.io"
#define MyAppURLSupport "https://github.com/NatronGitHub/Natron/issues"
#define MyAppURLUpdates "https://github.com/NatronGitHub/Natron/releases"
#define MyAppExeName "Natron.exe"
#define MyAppAssocName MyAppName + " Project"
#define MyAppAssocExt ".ntp"
#if NatronArch == "arm64"
; a ProgID has no spaces or parentheses
  #define MyAppAssocKey "NatronProjectARM64" + MyAppAssocExt
#else
  #define MyAppAssocKey StringChange(MyAppAssocName, " ", "") + MyAppAssocExt
#endif

[Setup]
AppId={#MyAppId}
AppName={#MyAppName}
AppVerName={#MyAppName}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURLSupport}
AppUpdatesURL={#MyAppURLUpdates}
DefaultDirName={autopf64}\{#MyAppName}
DefaultGroupName={#MyAppName}
PrivilegesRequiredOverridesAllowed=dialog
ChangesAssociations=yes
LicenseFile=LICENSE.txt
OutputDir=output
OutputBaseFilename=Setup
SetupIconFile=natronIcon256_windows.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern
WizardImageFile=WizardImageFile.bmp
WizardSmallImageFile=WizardSmallImageFile.bmp
WizardImageStretch=yes
#if NatronArch == "arm64"
; Native ARM64 binaries: only on Windows on Arm, installed in 64-bit mode (Program Files, the
; 64-bit registry view)
ArchitecturesAllowed=arm64
ArchitecturesInstallIn64BitMode=arm64
#endif

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Components]
Name: "natron"; Description: "Natron"; Types: full compact custom; Flags: fixed
;Name: "docs"; Description: "Natron Documentation"; Types: full
Name: "pyplugs"; Description: "Natron PyPlugs"; Types: full
Name: "ocio"; Description: "OpenColorIO Configs"; Types: full
Name: "io"; Description: "OpenFX IO"; Types: full compact custom; Flags: fixed
Name: "misc"; Description: "OpenFX Misc"; Types: full compact custom; Flags: fixed
Name: "cimg"; Description: "OpenFX CImg"; Types: full
Name: "stoy"; Description: "OpenFX Shadertoy"; Types: full
Name: "arena"; Description: "OpenFX Arena"; Types: full
Name: "gmic"; Description: "OpenFX G'MIC"; Types: full
#if NatronArch == "x64"
; Mesa's opengl32.dll: build-Windows-installer.sh only bundles one for x64 so far (#1494)
Name: "sgl"; Description: "OpenGL Software Renderer (FOR LEGACY HARDWARE ONLY)"
#endif

; Installing over an older Natron: remove the files whose names change between versions, so
; that no stale DLL, Qt plug-in or Python extension module stays behind: the DLLs of bin\ (not its
; .exe files, which may include the console scripts of the user's pip packages), the Qt plug-in
; directories, Mesa, the OFX plug-ins that Natron ships (not the user's own ones next to them),
; PySide/Shiboken and the Python zip. Not lib\, whose site-packages may hold the user's pip
; packages, nor the PyPlugs. Only if the directory is a Natron installation (IsNatronInstall), so
; that choosing an existing folder (e.g. D:\Tools) never deletes its bin\.
[InstallDelete]
Type: files; Name: "{app}\bin\*.dll"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\audio"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\bearer"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\generic"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\geometryloaders"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\iconengines"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\imageformats"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\mediaservice"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\multimedia"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\networkinformation"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\platforminputcontexts"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\platforms"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\platformthemes"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\playlistformats"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\position"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\printsupport"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\qmltooling"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\renderers"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\renderplugins"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\sceneparsers"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\sensors"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\sqldrivers"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\styles"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\texttospeech"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\tls"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\designer"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\virtualkeyboard"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\assetimporters"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\bin\mesa"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\Plugins\OFX\Natron\Arena.ofx.bundle"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\Plugins\OFX\Natron\CImg.ofx.bundle"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\Plugins\OFX\Natron\GMIC.ofx.bundle"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\Plugins\OFX\Natron\IO.ofx.bundle"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\Plugins\OFX\Natron\Misc.ofx.bundle"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\Plugins\OFX\Natron\Shadertoy.ofx.bundle"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\Plugins\PySide"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\Plugins\PySide2"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\Plugins\shiboken2"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\Plugins\PySide6"; Check: IsNatronInstall
Type: filesandordirs; Name: "{app}\Plugins\shiboken6"; Check: IsNatronInstall
Type: files; Name: "{app}\lib\python*.zip"; Check: IsNatronInstall

[Files]
Source: "{src}\bin\*"; DestDir: "{app}\bin\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\docs\*"; DestDir: "{app}\docs\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\lib\*"; DestDir: "{app}\lib\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external

Source: "{src}\Plugins\PySide\*"; DestDir: "{app}\Plugins\PySide\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external skipifsourcedoesntexist
Source: "{src}\Plugins\PySide2\*"; DestDir: "{app}\Plugins\PySide2\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external skipifsourcedoesntexist
Source: "{src}\Plugins\shiboken2\*"; DestDir: "{app}\Plugins\shiboken2\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external skipifsourcedoesntexist
Source: "{src}\Plugins\PySide6\*"; DestDir: "{app}\Plugins\PySide6\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external skipifsourcedoesntexist
Source: "{src}\Plugins\shiboken6\*"; DestDir: "{app}\Plugins\shiboken6\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external skipifsourcedoesntexist
Source: "{src}\Plugins\PyPlugs\*"; DestDir: "{app}\Plugins\PyPlugs\"; Components: pyplugs; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Plugins\OFX\Natron\Arena.ofx.bundle\*"; DestDir: "{app}\Plugins\OFX\Natron\Arena.ofx.bundle\"; Components: arena; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Plugins\OFX\Natron\CImg.ofx.bundle\*"; DestDir: "{app}\Plugins\OFX\Natron\CImg.ofx.bundle\"; Components: cimg; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Plugins\OFX\Natron\GMIC.ofx.bundle\*"; DestDir: "{app}\Plugins\OFX\Natron\GMIC.ofx.bundle\"; Components: gmic; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Plugins\OFX\Natron\IO.ofx.bundle\*"; DestDir: "{app}\Plugins\OFX\Natron\IO.ofx.bundle\"; Components: io; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Plugins\OFX\Natron\Misc.ofx.bundle\*"; DestDir: "{app}\Plugins\OFX\Natron\Misc.ofx.bundle\"; Components: misc; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Plugins\OFX\Natron\Shadertoy.ofx.bundle\*"; DestDir: "{app}\Plugins\OFX\Natron\Shadertoy.ofx.bundle\"; Components: stoy; Flags: ignoreversion recursesubdirs createallsubdirs external

Source: "{src}\Resources\docs\*"; DestDir: "{app}\Resources\docs\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Resources\etc\*"; DestDir: "{app}\Resources\etc\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Resources\OpenColorIO-Configs\*"; DestDir: "{app}\Resources\OpenColorIO-Configs\"; Components: ocio; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Resources\pixmaps\*"; DestDir: "{app}\Resources\pixmaps\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Resources\poppler\*"; DestDir: "{app}\Resources\poppler\"; Components: arena; Flags: ignoreversion recursesubdirs createallsubdirs external skipifsourcedoesntexist
Source: "{src}\Resources\stylesheets\*"; DestDir: "{app}\Resources\stylesheets\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external

#if NatronArch == "x64"
Source: "{src}\bin\mesa\opengl32.dll"; DestDir: "{app}\bin\"; Components: sgl; Flags: ignoreversion external skipifsourcedoesntexist
#endif

[Registry]
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocExt}\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppAssocKey}"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppAssocName}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\Resources\pixmaps\natronProjectIcon_windows.ico,0"
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\{#MyAppExeName}"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: "{#MyAppAssocExt}"; ValueData: ""

[Icons]
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{group}\{#MyAppName}"; Filename: "{app}\bin\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\bin\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
// True if {app} already has a Natron installation (for [InstallDelete])
function IsNatronInstall: Boolean;
begin
  Result := FileExists(ExpandConstant('{app}\bin\Natron.exe'));
end;

// Natron 2.5 and older were installed by the Qt Installer Framework (RB-2.5
// tools/jenkins/include/config/Windows.xml): each version in its own folder,
// @ApplicationsDir@\INRIA\Natron-<version> (Program Files), with its own maintenance tool
// (NatronSetup.exe) and its own uninstall entry, HKLM or HKCU
// Software\Microsoft\Windows\CurrentVersion\Uninstall\{<random ProductUUID>}, with
// DisplayName=Natron, Publisher=INRIA, InstallLocation and UninstallString=...\NatronSetup.exe.
// They stay installed next to this one. Setup lists them on an information page, and never
// installs into one of their folders: its files would be mixed with theirs, and [InstallDelete]
// would delete their DLLs.
const
  UninstallKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall';

var
  OldNatronDirs: TArrayOfString;
  OldNatronNames: TArrayOfString;
  OldNatronPage: TOutputMsgWizardPage;

// True if Dir is Parent or a folder inside it
function IsSameOrInsideDir(const Dir, Parent: String): Boolean;
begin
  Result := Pos(Lowercase(AddBackslash(Parent)), Lowercase(AddBackslash(Dir))) = 1;
end;

// True if Dir looks like a Natron installed by the Qt Installer Framework (2.5 or older)
function IsOldNatronInstall(const Dir: String): Boolean;
var
  D: String;
begin
  D := AddBackslash(Dir);
  Result := FileExists(D + 'bin\Natron.exe') and
    (FileExists(D + 'NatronSetup.exe') or FileExists(D + 'maintenancetool.exe') or
     FileExists(D + 'components.xml'));
end;

procedure AddOldNatron(Dir: String; const Name: String);
var
  I, N: Integer;
begin
  Dir := RemoveBackslashUnlessRoot(RemoveQuotes(Trim(Dir)));
  if (Dir = '') or not DirExists(Dir) then
    Exit;
  N := GetArrayLength(OldNatronDirs);
  for I := 0 to N - 1 do
    if CompareText(OldNatronDirs[I], Dir) = 0 then
      Exit;
  SetArrayLength(OldNatronDirs, N + 1);
  SetArrayLength(OldNatronNames, N + 1);
  OldNatronDirs[N] := Dir;
  OldNatronNames[N] := Name;
end;

// The uninstall entries of the Qt Installer Framework Natron (not this Setup's, whose key ends
// with _is1 and whose Publisher is MyAppPublisher)
procedure FindOldNatronInRegistry(const RootKey: Integer);
var
  Keys: TArrayOfString;
  I: Integer;
  Key, DisplayName, Publisher, UninstallString, Location, Version: String;
begin
  if not RegGetSubkeyNames(RootKey, UninstallKey, Keys) then
    Exit;
  for I := 0 to GetArrayLength(Keys) - 1 do begin
    Key := UninstallKey + '\' + Keys[I];
    if not RegQueryStringValue(RootKey, Key, 'DisplayName', DisplayName) then
      Continue;
    if Pos('natron', Lowercase(DisplayName)) <> 1 then
      Continue;
    if not RegQueryStringValue(RootKey, Key, 'Publisher', Publisher) then
      Publisher := '';
    if not RegQueryStringValue(RootKey, Key, 'UninstallString', UninstallString) then
      UninstallString := '';
    if (CompareText(Publisher, 'INRIA') <> 0) and
       (Pos('natronsetup.exe', Lowercase(UninstallString)) = 0) then
      Continue;
    if not RegQueryStringValue(RootKey, Key, 'InstallLocation', Location) then
      Continue;
    if not RegQueryStringValue(RootKey, Key, 'DisplayVersion', Version) then
      Version := '';
    if Version <> '' then
      DisplayName := DisplayName + ' ' + Version;
    AddOldNatron(Location, DisplayName);
  end;
end;

// The default folders of the Qt Installer Framework Natron: <ProgramFiles>\INRIA\Natron*
procedure FindOldNatronInFolder(const ProgramFiles: String);
var
  FindRec: TFindRec;
  Dir: String;
begin
  if FindFirst(ProgramFiles + '\INRIA\Natron*', FindRec) then begin
    try
      repeat
        if FindRec.Attributes and FILE_ATTRIBUTE_DIRECTORY <> 0 then begin
          Dir := ProgramFiles + '\INRIA\' + FindRec.Name;
          if IsOldNatronInstall(Dir) then
            AddOldNatron(Dir, FindRec.Name);
        end;
      until not FindNext(FindRec);
    finally
      FindClose(FindRec);
    end;
  end;
end;

// True if Dir is (or is inside) the folder of a Natron 2.5 or older
function IsOldNatronDir(const Dir: String): Boolean;
var
  I: Integer;
begin
  Result := IsOldNatronInstall(Dir);
  for I := 0 to GetArrayLength(OldNatronDirs) - 1 do
    if IsSameOrInsideDir(Dir, OldNatronDirs[I]) then
      Result := True;
end;

function OldNatronDirMessage(const Dir: String): String;
begin
  Result := 'The folder ' + Dir + ' holds Natron 2.5 or older.' + #13#10#13#10 +
    'Setup never installs Natron into the folder of an older version. Choose another folder.';
end;

procedure InitializeWizard;
var
  I: Integer;
  List: String;
begin
  FindOldNatronInRegistry(HKLM32);
  FindOldNatronInRegistry(HKCU32);
  FindOldNatronInFolder(ExpandConstant('{commonpf32}'));
  if IsWin64 then begin
    FindOldNatronInRegistry(HKLM64);
    FindOldNatronInRegistry(HKCU64);
    FindOldNatronInFolder(ExpandConstant('{commonpf64}'));
  end;
  if GetArrayLength(OldNatronDirs) = 0 then
    Exit;
  List := '';
  for I := 0 to GetArrayLength(OldNatronDirs) - 1 do begin
    List := List + '    ' + OldNatronNames[I] + ', in ' + OldNatronDirs[I] + #13#10;
    Log('Older Natron found: ' + OldNatronNames[I] + ', in ' + OldNatronDirs[I]);
  end;
  OldNatronPage := CreateOutputMsgPage(wpLicense, 'Older versions of Natron',
    'Natron 2.5 or older is installed on this computer.',
    'Setup found these older versions of Natron:' + #13#10#13#10 + List + #13#10 +
    'They stay installed. This version of Natron is installed next to them, in its own ' +
    'folder. Setup never installs it into the folder of an older version.' + #13#10#13#10 +
    'To remove an older version, open Settings > Apps > Installed apps (Apps & features on ' +
    'Windows 10), select the Natron published by INRIA, and select Uninstall.' + #13#10#13#10 +
    'Double-clicking a .ntp project file may still open the older version. To change this, ' +
    'right-click a .ntp file, and select Open with.');
end;

// The folder page refuses the folder of an older Natron
function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if (CurPageID = wpSelectDir) and IsOldNatronDir(WizardDirValue) then begin
    SuppressibleMsgBox(OldNatronDirMessage(WizardDirValue), mbError, MB_OK, IDOK);
    Result := False;
  end;
end;

// The same check when the folder page is skipped: silent install, /DIR=, or an update that
// reuses the folder of the previous installation
function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  if IsOldNatronDir(ExpandConstant('{app}')) then
    Result := OldNatronDirMessage(ExpandConstant('{app}'));
end;

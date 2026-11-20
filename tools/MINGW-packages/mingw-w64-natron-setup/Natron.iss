; NATRON WINDOWS INSTALLER
; Bump the pkgrel of PKGBUILD with every change of this file.

#define MyAppName "Natron"
#define MyAppPublisher "The Natron developers"
#define MyAppURL "https://natrongithub.github.io"
#define MyAppURLSupport "https://github.com/NatronGitHub/Natron/issues"
#define MyAppURLUpdates "https://github.com/NatronGitHub/Natron/releases"
#define MyAppExeName "Natron.exe"
#define MyAppAssocName MyAppName + " Project"
#define MyAppAssocExt ".ntp"
#define MyAppAssocKey StringChange(MyAppAssocName, " ", "") + MyAppAssocExt

[Setup]
AppId={{B1D54C04-B15B-4015-831A-CA5DACA60BD0}
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
Name: "sgl"; Description: "OpenGL Software Renderer (FOR LEGACY HARDWARE ONLY)"

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

Source: "{src}\bin\mesa\opengl32.dll"; DestDir: "{app}\bin\"; Components: sgl; Flags: ignoreversion external skipifsourcedoesntexist

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

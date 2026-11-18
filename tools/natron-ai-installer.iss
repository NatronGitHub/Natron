; Inno Setup script for Natron with the AI Assistant panel.
;
; Build with:
;     "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" tools\natron-ai-installer.iss
;
; Expects tools\package-natron-ai.sh to have been run first, so that
; dist\NatronAI\ exists.
;
; The shortcuts point straight at bin\Natron.exe with no wrapper script and no
; environment variables. That is safe because Natron resolves both of the things
; it needs relative to its own location:
;   Engine/OfxHost.cpp:890-892   <exe>/../Plugins/OFX/Natron   (OpenFX plug-ins)
;   Engine/Settings.cpp:100      <exe>/../Resources/OpenColorIO-Configs
; Verified empirically: 162 plug-ins load with OFX_PLUGIN_PATH and OCIO unset.
; Using a .bat launcher instead would flash a console window on every start.

#define AppName "Natron AI"
#define AppVersion "2.6.0-ai"
#define AppPublisher "Natron AI build"
#define AppExeName "Natron.exe"
#define SourceDir "..\dist\NatronAI"

[Setup]
AppId={{7F3A1C42-9B5E-4D18-A6C7-2E9D4B8F1A30}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\NatronAI
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=NatronAI-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; ~400 MB of payload plus working room.
ExtraDiskSpaceRequired=10485760
UninstallDisplayIcon={app}\bin\{#AppExeName}
LicenseFile=..\LICENSE.txt
PrivilegesRequired=admin
; Let a user without admin rights install into their own profile instead of
; failing outright; the app needs no system-wide state.
PrivilegesRequiredOverridesAllowed=dialog

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Files]
; Everything the packaging script produced. recursesubdirs pulls in bin\,
; lib\python3.14\, Plugins\ and Resources\ as they are.
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\bin\{#AppExeName}"; WorkingDir: "{app}\bin"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\bin\{#AppExeName}"; WorkingDir: "{app}\bin"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Natron writes an OpenFX plug-in cache next to the plug-ins on first run; it is
; not part of the install manifest, so remove it explicitly or the directory is
; left behind.
Type: filesandordirs; Name: "{app}\Plugins\OFX\Natron\*.cache"
Type: dirifempty; Name: "{app}\Plugins\OFX\Natron"
Type: dirifempty; Name: "{app}\Plugins\OFX"
Type: dirifempty; Name: "{app}\Plugins"
Type: dirifempty; Name: "{app}"

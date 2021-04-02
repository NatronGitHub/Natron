; NATRON WINDOWS INSTALLER

#define MyAppName "Natron"
#define MyAppPublisher "Natron"
#define MyAppURL "https://natrongithub.github.io"
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
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
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

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Components]
Name: "natron"; Description: "Natron"; Types: full compact custom; Flags: fixed
Name: "docs"; Description: "Natron Documentation"; Types: full
Name: "pyplugs"; Description: "Natron PyPlugs"; Types: full
Name: "ocio"; Description: "OpenColorIO Configs"; Types: full
Name: "io"; Description: "OpenFX IO"; Types: full
Name: "misc"; Description: "OpenFX Misc"; Types: full
Name: "cimg"; Description: "OpenFX CImg"; Types: full
Name: "stoy"; Description: "OpenFX Shadertoy"; Types: full
Name: "arena"; Description: "OpenFX Arena"; Types: full
Name: "gmic"; Description: "OpenFX G'MIC"; Types: full

[Files]
Source: "{src}\bin\*"; DestDir: "{app}\bin\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\docs\*"; DestDir: "{app}\docs\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\lib\*"; DestDir: "{app}\lib\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external

Source: "{src}\Plugins\PySide\*"; DestDir: "{app}\Plugins\PySide\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Plugins\PyPlugs\*"; DestDir: "{app}\Plugins\PyPlugs\"; Components: pyplugs; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Plugins\OFX\Natron\Arena.ofx.bundle\*"; DestDir: "{app}\Plugins\OFX\Natron\Arena.ofx.bundle\"; Components: arena; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Plugins\OFX\Natron\CImg.ofx.bundle\*"; DestDir: "{app}\Plugins\OFX\Natron\CImg.ofx.bundle\"; Components: cimg; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Plugins\OFX\Natron\GMIC.ofx.bundle\*"; DestDir: "{app}\Plugins\OFX\Natron\GMIC.ofx.bundle\"; Components: gmic; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Plugins\OFX\Natron\IO.ofx.bundle\*"; DestDir: "{app}\Plugins\OFX\Natron\IO.ofx.bundle\"; Components: io; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Plugins\OFX\Natron\Misc.ofx.bundle\*"; DestDir: "{app}\Plugins\OFX\Natron\Misc.ofx.bundle\"; Components: misc; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Plugins\OFX\Natron\Shadertoy.ofx.bundle\*"; DestDir: "{app}\Plugins\OFX\Natron\Shadertoy.ofx.bundle\"; Components: stoy; Flags: ignoreversion recursesubdirs createallsubdirs external

Source: "{src}\Resources\docs\*"; DestDir: "{app}\Resources\docs\"; Components: docs; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Resources\etc\*"; DestDir: "{app}\Resources\etc\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Resources\OpenColorIO-Configs\*"; DestDir: "{app}\Resources\OpenColorIO-Configs\"; Components: ocio; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Resources\pixmaps\*"; DestDir: "{app}\Resources\pixmaps\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Resources\poppler\*"; DestDir: "{app}\Resources\poppler\"; Components: arena; Flags: ignoreversion recursesubdirs createallsubdirs external
Source: "{src}\Resources\stylesheets\*"; DestDir: "{app}\Resources\stylesheets\"; Components: natron; Flags: ignoreversion recursesubdirs createallsubdirs external

[Registry]
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocExt}\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppAssocKey}"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppAssocName}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\Resources\pixmaps\natronProjectIcon_windows.ico,0"
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\{#MyAppExeName}"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: "{#MyAppAssocExt}"; ValueData: ""

[Icons]
;Name: "{group}\{cm:ProgramOnTheWeb,{#MyAppName}}"; Filename: "{#MyAppURL}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{group}\{#MyAppName}"; Filename: "{app}\bin\{#MyAppExeName}"
Name: "{group}\{#MyAppName}"; Filename: "{app}\bin\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent


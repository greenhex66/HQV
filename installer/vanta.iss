[Setup]
AppId={{4E2B7C91-6D3A-4F08-A1C5-9B7E3D2F6A10}
AppName=HEX-Q VANTA
AppVersion=1.0.0
AppPublisher=HEX-Q
DefaultDirName={autopf}\HEX-Q VANTA
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..
OutputBaseFilename=HEX-Q-VANTA-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Files]
Source: "..\dist\HEX-Q VANTA.vst3\*"; DestDir: "{commoncf64}\VST3\HEX-Q VANTA.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

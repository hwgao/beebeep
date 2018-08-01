; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{85A0D6B6-2163-425F-92E7-E0BC4CEDCE9A}
AppName=BeeBEEP
AppVersion=5.2.1
;AppVerName=BeeBEEP 5.2.1
AppPublisher=Marco Mastroddi Software
AppPublisherURL=http://www.beebeep.net/
AppSupportURL=http://www.beebeep.net/
AppUpdatesURL=http://www.beebeep.net/
DefaultDirName={pf}\BeeBEEP
DisableProgramGroupPage=yes
OutputBaseFilename=beebeep-setup-5.2.1
SetupIconFile=..\src\beebeep.ico
Compression=lzma
SolidCompression=yes
AppCopyright=Copyright (C) Marco Mastroddi
OutputDir=.
LicenseFile=..\misc\license.txt

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Files]
Source: "..\..\beebeep-5.2.1\beebeep.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\beebeep-5.2.1\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{commonprograms}\BeeBEEP"; Filename: "{app}\beebeep.exe"
Name: "{commondesktop}\BeeBEEP"; Filename: "{app}\beebeep.exe"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\BeeBEEP"; Filename: "{app}\beebeep.exe"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\beebeep.exe"; Description: "{cm:LaunchProgram,BeeBEEP}"; Flags: nowait postinstall skipifsilent


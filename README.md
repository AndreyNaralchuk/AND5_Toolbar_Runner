# AND5_Toolbar_Runner

A Windows system tray toolbar. The tray icon opens a menu. The menu is built from `AND5_toolbar.xml`. A menu item runs a script or program as administrator.

It is a port of the PowerShell script `AND5_SCRIPTS_HOME\AND5_com\toolbar\1_AND5_create_toolbar_on_Logon.ps1` to plain C and the Win32 API.

**Status: working.** Open items: single instance, and XML reload without restart.

Full documentation: `F:\DEV_HOME\AI_Projects\AND5_Toolbar_Runner\README_FULL.md`

## Download

**Latest version: [v2026.11.25](https://github.com/AndreyNaralchuk/AND5_Toolbar_Runner/releases/tag/v2026.11.25)**

| Platform | File | Instruction |
|---|---|---|
| Windows x64 | [Download `.exe`](https://github.com/AndreyNaralchuk/AND5_Toolbar_Runner/releases/latest/download/AND5_Toolbar_Runner.exe) | Run `AND5_Toolbar_Runner.exe`. One file, no installer. |

**All versions:** [GitHub Releases](https://github.com/AndreyNaralchuk/AND5_Toolbar_Runner/releases)

## UI example

The tray menu of the running program:

![UI example](Images_From_Running/UI_Example.png)

### Install with PowerShell

Run in a normal PowerShell window. The script downloads the program to `%LOCALAPPDATA%\AND5_Toolbar_Runner`, adds a shortcut to autostart and starts the program.

```powershell
$dir = Join-Path $env:LOCALAPPDATA 'AND5_Toolbar_Runner'
$exe = Join-Path $dir 'AND5_Toolbar_Runner.exe'
New-Item -ItemType Directory -Force -Path $dir | Out-Null

# Download the latest release
Invoke-WebRequest -Uri 'https://github.com/AndreyNaralchuk/AND5_Toolbar_Runner/releases/latest/download/AND5_Toolbar_Runner.exe' -OutFile $exe

# Autostart shortcut
$lnk = Join-Path ([Environment]::GetFolderPath('Startup')) 'AND5_Toolbar_Runner.lnk'
$sc = (New-Object -ComObject WScript.Shell).CreateShortcut($lnk)
$sc.TargetPath = $exe
$sc.WorkingDirectory = $dir
$sc.Save()

# Start now
Start-Process $exe
```

The program needs the variable `AND5_SCRIPTS_HOME` (folder with `AND5_toolbar.xml`), or a path to the XML file as the first argument. Set the variable once:

```powershell
[Environment]::SetEnvironmentVariable('AND5_SCRIPTS_HOME', 'D:\path\AND5_SCRIPTS_HOME', 'User')
```

To remove the program: delete the folder `%LOCALAPPDATA%\AND5_Toolbar_Runner` and the shortcut `AND5_Toolbar_Runner.lnk` in `shell:startup`.

## Features

- Tray icon (`Shell_NotifyIconW`), tooltip `Andreys TOOLBAR`.
- Left and right click open the same menu.
- Menu and submenu (one level) come from XML. Items have `.ico` icons.
- `Separator` gives a divider. `About` shows the About window. `Exit` closes the program.
- An item with `<command>` starts `powershell.exe` as administrator (`runas`).
- The icon comes back after an Explorer restart (`TaskbarCreated`).

## Tech stack

- C17, no C++, no runtime.
- Win32 API, XmlLite (DTD prohibited), `ShellExecuteExW`.
- CMake + MSVC (Visual Studio 2022), x64, `/MT`, Unicode, `/W4`.

## AND5_toolbar.xml

Minimal example `AND5_SCRIPTS_HOME\AND5_toolbar.xml`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<toolbar>
	<menu>
		<name>About</name>
		<icon>AND5_com\toolbar\about.ico</icon>
		<submenu>
			<name>About</name>
			<icon>AND5_com\toolbar\about.ico</icon>
		</submenu>
		<submenu>
			<name>Exit</name>
			<icon>AND5_com\toolbar\exit.ico</icon>
		</submenu>
	</menu>
	<menu>
		<name>Separator</name>
		<icon />
	</menu>
	<menu>
		<name>Task Manager</name>
		<icon>AND5_com\daily_scripts\Task_Manager.ico</icon>
		<command>AND5_SCRIPTS_ROOT#AND5_com\daily_scripts\Task_Manager.bat</command>
	</menu>
</toolbar>
```

- `<command>` format: `VARIABLE#relative\path`. Without `VARIABLE#` the path is relative to `%AND5_SCRIPTS_ROOT%`.
- `<icon>` is relative to `%AND5_SCRIPTS_ROOT%`. It can be empty.

### Minimum for a working file

- Needed: one `<menu>` with `<name>`. The root tag name is not checked.
- Not needed: `<icon>`, `<command>`, `<submenu>`.
- Not allowed: `<!DOCTYPE>` (DTD is prohibited).
- `<name>Exit</name>` without `<command>` closes the program. `<name>About</name>` without `<command>` shows the About window.
- An item with a name but no `<command>` (and not About or Exit) is shown, but a click does nothing.
- If the XML is broken, missing, or has no `<menu>`, the program shows a fallback menu with `About` and `Exit` only.

Smallest useful file:

```xml
<toolbar>
	<menu>
		<name>Task Manager</name>
		<command>AND5_com\daily_scripts\Task_Manager.bat</command>
	</menu>
	<menu>
		<name>Exit</name>
	</menu>
</toolbar>
```

## Environment variables

| Variable | Purpose |
|---|---|
| `AND5_SCRIPTS_ROOT` | Scripts root. Icons and commands are relative to it. |
| `AND5_SCRIPTS_HOME` | Folder with `AND5_toolbar.xml`. |

The XML path is the first command-line argument. If there is none, it is `%AND5_SCRIPTS_HOME%\AND5_toolbar.xml`.

## Source layout

`src\` has `AND5_main.c`, `AND5_config.c`, `AND5_xml.c`, `AND5_icon.c`, `AND5_menu.c`, `AND5_tray.c`, `AND5_exec.c`, `AND5_app.h`, `AND5_app.rc`, `AND5_app.manifest`.

## Program structure

### Modules

Arrows show which module calls which. All modules share the types and the `gl_AND5_AppConfig` variable from `AND5_app.h`.

```mermaid
flowchart TD
    ENV[("Environment<br/>AND5_SCRIPTS_ROOT<br/>AND5_SCRIPTS_HOME")]
    ARG[/"Command line<br/>argv 1 = XML path"/]
    XMLF[("AND5_toolbar.xml")]
    ICO[("*.ico files")]

    subgraph EXE["AND5_Toolbar_Runner.exe"]
        MAIN["AND5_main.c<br/>wWinMain, AND5_AppWndProc<br/>AND5_AppLoad, AND5_AppUnload"]
        CFG["AND5_config.c<br/>AND5_CfgInit<br/>AND5_CfgResolveIcon<br/>AND5_CfgResolveCommand"]
        XML["AND5_xml.c<br/>AND5_XmlLoad, AND5_XmlFree<br/>XmlLite parser"]
        MENU["AND5_menu.c<br/>AND5_MenuBuild, AND5_MenuShow<br/>AND5_MenuFind, AND5_MenuDestroy"]
        ICON["AND5_icon.c<br/>AND5_IcoLoadBitmap"]
        TRAY["AND5_tray.c<br/>AND5_TrayAdd, AND5_TrayRemove"]
        EXEC["AND5_exec.c<br/>AND5_ExecItem"]
        APP["AND5_app.h<br/>AND5_TB_ITEM, AND5_TB_KIND<br/>AND5_CONFIG, constants"]
        RES["AND5_app.rc, AND5_resource.h<br/>AND5_app.manifest<br/>IDI_APP icon"]
    end

    PS["powershell.exe<br/>runas, administrator"]
    SCR["bat and ps1 scripts"]
    SHELL["Windows shell<br/>tray, popup menu"]

    ARG --> CFG
    ENV --> CFG
    MAIN --> CFG
    MAIN --> XML
    MAIN --> MENU
    MAIN --> TRAY
    MAIN --> EXEC
    XML --> XMLF
    MENU --> ICON
    MENU --> CFG
    ICON --> ICO
    EXEC --> CFG
    EXEC --> PS
    PS --> SCR
    TRAY --> SHELL
    MENU --> SHELL
    RES -.-> MAIN
    APP -.-> MAIN

    style MAIN fill:#DBEAFE,stroke:#2563EB,color:#172554
    style CFG fill:#FEF3C7,stroke:#D97706,color:#422006
    style XML fill:#DCFCE7,stroke:#16A34A,color:#14532D
    style MENU fill:#DCFCE7,stroke:#16A34A,color:#14532D
    style ICON fill:#DCFCE7,stroke:#16A34A,color:#14532D
    style TRAY fill:#DCFCE7,stroke:#16A34A,color:#14532D
    style EXEC fill:#FEE2E2,stroke:#DC2626,color:#7F1D1D
    style APP fill:#F3F4F6,stroke:#6B7280,color:#111827
    style RES fill:#F3F4F6,stroke:#6B7280,color:#111827
```

### Runtime flow

```mermaid
sequenceDiagram
    participant U as User
    participant M as AND5_main.c
    participant C as AND5_config.c
    participant X as AND5_xml.c
    participant B as AND5_menu.c
    participant T as AND5_tray.c
    participant E as AND5_exec.c

    M->>C: AND5_CfgInit(argv 1)
    M->>M: AND5_AppLoadIcon (AND5_ok.ico or IDI_APP)
    M->>M: RegisterClassExW, CreateWindowExW
    Note over M: WM_CREATE
    M->>X: AND5_XmlLoad(XmlFile)
    X-->>M: AND5_TB_ITEM tree
    M->>B: AND5_MenuBuild(tree)
    M->>T: AND5_TrayAdd(icon)
    U->>T: click on tray icon
    T-->>M: AND5_WM_TRAY
    M->>B: AND5_MenuShow
    U->>B: choose item
    B-->>M: WM_COMMAND(Id)
    M->>B: AND5_MenuFind(Id)
    M->>E: AND5_ExecItem(item)
    alt AND5_TbCommand
        E->>C: AND5_CfgResolveCommand
        E->>E: ShellExecuteExW runas powershell.exe
    else AND5_TbAbout
        E->>U: About window
    else AND5_TbExit
        E->>M: DestroyWindow
        Note over M: WM_DESTROY<br/>AND5_TrayRemove, AND5_AppUnload
    end
```

### Data model

```mermaid
classDiagram
    class AND5_TB_ITEM {
        AND5_TB_KIND Kind
        WCHAR Name
        WCHAR Icon
        WCHAR Command
        UINT Id
        HBITMAP Bitmap
        AND5_TB_ITEM* Items
        ULONG Count
    }
    class AND5_TB_KIND {
        <<enumeration>>
        AND5_TbNone
        AND5_TbCommand
        AND5_TbSeparator
        AND5_TbAbout
        AND5_TbExit
    }
    class AND5_CONFIG {
        WCHAR Root
        WCHAR Home
        WCHAR XmlFile
    }
    AND5_TB_ITEM --> AND5_TB_KIND : Kind
    AND5_TB_ITEM "1" o-- "0..*" AND5_TB_ITEM : Items, submenu
```

## Build

Needs Visual Studio 2022 with the C++ desktop workload. Run in Developer PowerShell:

```powershell
cmake -S F:\DEV_HOME\AI_Projects\AND5_Toolbar_Runner -B F:\DEV_HOME\AI_Projects\AND5_Toolbar_Runner\build -G "Visual Studio 17 2022" -A x64
cmake --build F:\DEV_HOME\AI_Projects\AND5_Toolbar_Runner\build --config Release
```

Result: `bin\AND5_Toolbar_Runner.exe`. One file, no installer.

## Run

```powershell
AND5_Toolbar_Runner.exe                     # XML from %AND5_SCRIPTS_HOME%
AND5_Toolbar_Runner.exe D:\path\my.xml      # custom XML
```

For autostart, put a shortcut in `shell:startup` or use Task Scheduler.

## TODO

- [ ] Single instance (named mutex).
- [ ] Reload XML without restart.

## License

MIT License. Copyright (c) 2026 Andrey Naralchuk. See [LICENSE](LICENSE).

The MIT License covers the source code of this project. Third-party programs in `AND5_SCRIPTS_HOME\BIN` and `example` (Sysinternals tools such as Autoruns, Process Explorer, TCPView, Procmon) belong to Microsoft and have their own license terms.

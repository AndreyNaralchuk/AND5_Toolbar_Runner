# AND5_Toolbar_Runner

A Windows system tray toolbar. The tray icon opens a menu. The menu is built from `AND5_toolbar.xml`. A menu item runs a script or program as administrator.

It is a port of the PowerShell script `AND5_SCRIPTS_HOME\AND5_com\toolbar\1_AND5_create_toolbar_on_Logon.ps1` to plain C and the Win32 API.

**Status: working.** Open items: single instance, and XML reload without restart.

Full documentation: `F:\DEV_HOME\AI_Projects\AND5_Toolbar_Runner\README_FULL.md`

## UI example

The tray menu of the running program:

![UI example](Images_From_Running/UI_Example.png)

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

```xml
<toolbar>
	<menu>
		<name>TC_DATA</name>
		<icon>AND5_com\toolbar\folder.ico</icon>
		<command>AND5_SCRIPTS_ROOT#AND5_com\daily_scripts\TC_DATA.bat</command>
		<submenu>
			<name>model.xml</name>
			<icon>AND5_com\toolbar\xml.ico</icon>
			<command>AND5_SCRIPTS_ROOT#AND5_com\toolbar\201_open_model_xml.bat</command>
		</submenu>
	</menu>
</toolbar>
```

- `<command>` format: `VARIABLE#relative\path`.
- `<icon>` is relative to `%AND5_SCRIPTS_ROOT%`. It can be empty.

## Environment variables

| Variable | Purpose |
|---|---|
| `AND5_SCRIPTS_ROOT` | Scripts root. Icons and commands are relative to it. |
| `AND5_SCRIPTS_HOME` | Folder with `AND5_toolbar.xml`. |

The XML path is the first command-line argument. If there is none, it is `%AND5_SCRIPTS_HOME%\AND5_toolbar.xml`.

## Source layout

`src\` has `main.c`, `config.c`, `xml.c`, `icon.c`, `menu.c`, `tray.c`, `exec.c`, `app.h`, `app.rc`, `app.manifest`.

## Program structure

### Modules

Arrows show which module calls which. All modules share the types and the `AppConfig` variable from `app.h`.

```mermaid
flowchart TD
    ENV[("Environment<br/>AND5_SCRIPTS_ROOT<br/>AND5_SCRIPTS_HOME")]
    ARG[/"Command line<br/>argv 1 = XML path"/]
    XMLF[("AND5_toolbar.xml")]
    ICO[("*.ico files")]

    subgraph EXE["AND5_Toolbar_Runner.exe"]
        MAIN["main.c<br/>wWinMain, AppWndProc<br/>AppLoad, AppUnload"]
        CFG["config.c<br/>CfgInit<br/>CfgResolveIcon<br/>CfgResolveCommand"]
        XML["xml.c<br/>XmlLoad, XmlFree<br/>XmlLite parser"]
        MENU["menu.c<br/>MenuBuild, MenuShow<br/>MenuFind, MenuDestroy"]
        ICON["icon.c<br/>IcoLoadBitmap"]
        TRAY["tray.c<br/>TrayAdd, TrayRemove"]
        EXEC["exec.c<br/>ExecItem"]
        APP["app.h<br/>TB_ITEM, TB_KIND<br/>APP_CONFIG, constants"]
        RES["app.rc, resource.h<br/>app.manifest<br/>IDI_APP icon"]
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
    participant M as main.c
    participant C as config.c
    participant X as xml.c
    participant B as menu.c
    participant T as tray.c
    participant E as exec.c

    M->>C: CfgInit(argv 1)
    M->>M: AppLoadIcon (ok.ico or IDI_APP)
    M->>M: RegisterClassExW, CreateWindowExW
    Note over M: WM_CREATE
    M->>X: XmlLoad(XmlFile)
    X-->>M: TB_ITEM tree
    M->>B: MenuBuild(tree)
    M->>T: TrayAdd(icon)
    U->>T: click on tray icon
    T-->>M: WM_TRAY
    M->>B: MenuShow
    U->>B: choose item
    B-->>M: WM_COMMAND(Id)
    M->>B: MenuFind(Id)
    M->>E: ExecItem(item)
    alt TbCommand
        E->>C: CfgResolveCommand
        E->>E: ShellExecuteExW runas powershell.exe
    else TbAbout
        E->>U: About window
    else TbExit
        E->>M: DestroyWindow
        Note over M: WM_DESTROY<br/>TrayRemove, AppUnload
    end
```

### Data model

```mermaid
classDiagram
    class TB_ITEM {
        TB_KIND Kind
        WCHAR Name
        WCHAR Icon
        WCHAR Command
        UINT Id
        HBITMAP Bitmap
        TB_ITEM* Items
        ULONG Count
    }
    class TB_KIND {
        <<enumeration>>
        TbNone
        TbCommand
        TbSeparator
        TbAbout
        TbExit
    }
    class APP_CONFIG {
        WCHAR Root
        WCHAR Home
        WCHAR XmlFile
    }
    TB_ITEM --> TB_KIND : Kind
    TB_ITEM "1" o-- "0..*" TB_ITEM : Items, submenu
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

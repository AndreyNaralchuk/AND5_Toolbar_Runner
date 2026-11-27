#pragma once

#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>
#include "AND5_resource.h"

#define AND5_APP_NAME L"AND5_Toolbar_Runner"
#define AND5_APP_TIP L"Andreys TOOLBAR"
#define AND5_APP_TRAY_ICON L"AND5_com\\toolbar\\AND5_ok.ico"
#define AND5_APP_XML_NAME L"AND5_toolbar.xml"
#define AND5_APP_ABOUT_TEXT L"This toolbar created by Andrey Naralchuk to speed up routine operations.\nCopyright 2026."
#define AND5_WM_TRAY (WM_APP + 1)
#define AND5_ID_FIRST 40000

typedef enum _AND5_TB_KIND
{
    AND5_TbNone,
    AND5_TbCommand,
    AND5_TbSeparator,
    AND5_TbAbout,
    AND5_TbExit
} AND5_TB_KIND;

typedef struct _AND5_TB_ITEM
{
    AND5_TB_KIND Kind;
    WCHAR Name[128];
    WCHAR Icon[MAX_PATH];
    WCHAR Command[MAX_PATH];
    UINT Id;
    HBITMAP Bitmap;
    struct _AND5_TB_ITEM *Items;
    ULONG Count;
} AND5_TB_ITEM, *PAND5_TB_ITEM;

typedef struct _AND5_CONFIG
{
    WCHAR Root[MAX_PATH];
    WCHAR Home[MAX_PATH];
    WCHAR XmlFile[MAX_PATH];
} AND5_CONFIG;

extern AND5_CONFIG gl_AND5_AppConfig;

// AND5_config.c
BOOL AND5_CfgInit(_In_opt_ PCWSTR p_XmlFile);
BOOL AND5_CfgResolveIcon(_In_ PCWSTR p_Icon, _Out_writes_(p_Cch) PWSTR p_Path, _In_ ULONG p_Cch);
BOOL AND5_CfgResolveCommand(_In_ PCWSTR p_Command, _Out_writes_(p_Cch) PWSTR p_Path, _In_ ULONG p_Cch);

// AND5_xml.c
BOOL AND5_XmlLoad(_In_ PCWSTR p_XmlFile, _Out_ PAND5_TB_ITEM p_Root);
VOID AND5_XmlFree(_Inout_ PAND5_TB_ITEM p_Root);

// AND5_icon.c
HBITMAP AND5_IcoLoadBitmap(_In_ PCWSTR p_Path, _In_ LONG p_Size);

// AND5_menu.c
HMENU AND5_MenuBuild(_Inout_ PAND5_TB_ITEM p_Root);
VOID AND5_MenuDestroy(_In_opt_ HMENU p_Menu, _Inout_ PAND5_TB_ITEM p_Root);
PAND5_TB_ITEM AND5_MenuFind(_In_ PAND5_TB_ITEM p_Root, _In_ UINT p_Id);
VOID AND5_MenuShow(_In_ HWND p_Window, _In_ HMENU p_Menu);

// AND5_tray.c
BOOL AND5_TrayAdd(_In_ HWND p_Window, _In_ HICON p_Icon);
VOID AND5_TrayRemove(_In_ HWND p_Window);

// AND5_exec.c
VOID AND5_ExecItem(_In_ HWND p_Window, _In_ PAND5_TB_ITEM p_Item);

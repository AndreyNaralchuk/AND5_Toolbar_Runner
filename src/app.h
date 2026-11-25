#pragma once

#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>
#include "resource.h"

#define APP_NAME L"AND5_Toolbar_Runner"
#define APP_TIP L"Andreys TOOLBAR"
#define APP_TRAY_ICON L"AND5_com\\toolbar\\ok.ico"
#define APP_XML_NAME L"AND5_toolbar.xml"
#define APP_ABOUT_TEXT L"This toolbar created by Andrey to speed up routine operations.\nCopyright 2023."
#define WM_TRAY (WM_APP + 1)
#define ID_FIRST 40000

typedef enum _TB_KIND
{
    TbNone,
    TbCommand,
    TbSeparator,
    TbAbout,
    TbExit
} TB_KIND;

typedef struct _TB_ITEM
{
    TB_KIND Kind;
    WCHAR Name[128];
    WCHAR Icon[MAX_PATH];
    WCHAR Command[MAX_PATH];
    UINT Id;
    HBITMAP Bitmap;
    struct _TB_ITEM *Items;
    ULONG Count;
} TB_ITEM, *PTB_ITEM;

typedef struct _APP_CONFIG
{
    WCHAR Root[MAX_PATH];
    WCHAR Home[MAX_PATH];
    WCHAR XmlFile[MAX_PATH];
} APP_CONFIG;

extern APP_CONFIG AppConfig;

// config.c
BOOL CfgInit(_In_opt_ PCWSTR XmlFile);
BOOL CfgResolveIcon(_In_ PCWSTR Icon, _Out_writes_(Cch) PWSTR Path, _In_ ULONG Cch);
BOOL CfgResolveCommand(_In_ PCWSTR Command, _Out_writes_(Cch) PWSTR Path, _In_ ULONG Cch);

// xml.c
BOOL XmlLoad(_In_ PCWSTR XmlFile, _Out_ PTB_ITEM Root);
VOID XmlFree(_Inout_ PTB_ITEM Root);

// icon.c
HBITMAP IcoLoadBitmap(_In_ PCWSTR Path, _In_ LONG Size);

// menu.c
HMENU MenuBuild(_Inout_ PTB_ITEM Root);
VOID MenuDestroy(_In_opt_ HMENU Menu, _Inout_ PTB_ITEM Root);
PTB_ITEM MenuFind(_In_ PTB_ITEM Root, _In_ UINT Id);
VOID MenuShow(_In_ HWND Window, _In_ HMENU Menu);

// tray.c
BOOL TrayAdd(_In_ HWND Window, _In_ HICON Icon);
VOID TrayRemove(_In_ HWND Window);

// exec.c
VOID ExecItem(_In_ HWND Window, _In_ PTB_ITEM Item);

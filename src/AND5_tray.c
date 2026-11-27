#include "AND5_app.h"

static VOID AND5_TrayInit(_Out_ NOTIFYICONDATAW *p_Data, _In_ HWND p_Window)
{
    ZeroMemory(p_Data, sizeof(NOTIFYICONDATAW));
    p_Data->cbSize = sizeof(NOTIFYICONDATAW);
    p_Data->hWnd = p_Window;
    p_Data->uID = 1;
}

BOOL AND5_TrayAdd(_In_ HWND p_Window, _In_ HICON p_Icon)
{
    NOTIFYICONDATAW nid;

    AND5_TrayInit(&nid, p_Window);
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = AND5_WM_TRAY;
    nid.hIcon = p_Icon;
    StringCchCopyW(nid.szTip, ARRAYSIZE(nid.szTip), AND5_APP_TIP);

    return Shell_NotifyIconW(NIM_ADD, &nid);
}

VOID AND5_TrayRemove(_In_ HWND p_Window)
{
    NOTIFYICONDATAW nid;

    AND5_TrayInit(&nid, p_Window);
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

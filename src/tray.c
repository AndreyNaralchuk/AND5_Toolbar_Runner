#include "app.h"

static VOID TrayInit(_Out_ NOTIFYICONDATAW *Data, _In_ HWND Window)
{
    ZeroMemory(Data, sizeof(NOTIFYICONDATAW));
    Data->cbSize = sizeof(NOTIFYICONDATAW);
    Data->hWnd = Window;
    Data->uID = 1;
}

BOOL TrayAdd(_In_ HWND Window, _In_ HICON Icon)
{
    NOTIFYICONDATAW nid;

    TrayInit(&nid, Window);
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAY;
    nid.hIcon = Icon;
    StringCchCopyW(nid.szTip, ARRAYSIZE(nid.szTip), APP_TIP);

    return Shell_NotifyIconW(NIM_ADD, &nid);
}

VOID TrayRemove(_In_ HWND Window)
{
    NOTIFYICONDATAW nid;

    TrayInit(&nid, Window);
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

#include "app.h"

static VOID ExecCommand(_In_ HWND Window, _In_ PCWSTR Command)
{
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    WCHAR path[MAX_PATH];
    WCHAR text[MAX_PATH + 64];
    WCHAR params[MAX_PATH + 32];
    DWORD attributes;

    if (!CfgResolveCommand(Command, path, MAX_PATH))
        StringCchCopyW(path, MAX_PATH, Command);

    attributes = GetFileAttributesW(path);

    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        StringCchPrintfW(text, ARRAYSIZE(text), L"Executable file is not found.\n%s", path);
        MessageBoxW(Window, text, L"Warning", MB_OK | MB_ICONWARNING);
        return;
    }

    StringCchPrintfW(params, ARRAYSIZE(params), L"-Command \"& '%s'\"", path);

    sei.hwnd = Window;
    sei.lpVerb = L"runas";
    sei.lpFile = L"powershell.exe";
    sei.lpParameters = params;
    sei.nShow = SW_SHOWNORMAL;

    // ERROR_CANCELLED: the user said No in the UAC prompt.
    if (!ShellExecuteExW(&sei) && GetLastError() != ERROR_CANCELLED)
    {
        StringCchPrintfW(text, ARRAYSIZE(text), L"Command is not started.\n%s", path);
        MessageBoxW(Window, text, L"Warning", MB_OK | MB_ICONWARNING);
    }
}

VOID ExecItem(_In_ HWND Window, _In_ PTB_ITEM Item)
{
    switch (Item->Kind)
    {
    case TbCommand:
        ExecCommand(Window, Item->Command);
        break;
    case TbAbout:
        MessageBoxW(Window, APP_ABOUT_TEXT, L"About", MB_OK | MB_ICONINFORMATION);
        break;
    case TbExit:
        DestroyWindow(Window);
        break;
    default:
        break;
    }
}

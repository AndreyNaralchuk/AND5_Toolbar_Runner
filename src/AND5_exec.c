#include "AND5_app.h"

static VOID AND5_ExecCommand(_In_ HWND p_Window, _In_ PCWSTR p_Command)
{
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    WCHAR path[MAX_PATH];
    WCHAR text[MAX_PATH + 64];
    WCHAR params[MAX_PATH + 32];
    DWORD attributes;

    if (!AND5_CfgResolveCommand(p_Command, path, MAX_PATH))
        StringCchCopyW(path, MAX_PATH, p_Command);

    attributes = GetFileAttributesW(path);

    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        StringCchPrintfW(text, ARRAYSIZE(text), L"Executable file is not found.\n%s", path);
        MessageBoxW(p_Window, text, L"Warning", MB_OK | MB_ICONWARNING);
        return;
    }

    StringCchPrintfW(params, ARRAYSIZE(params), L"-Command \"& '%s'\"", path);

    sei.hwnd = p_Window;
    sei.lpVerb = L"runas";
    sei.lpFile = L"powershell.exe";
    sei.lpParameters = params;
    sei.nShow = SW_SHOWNORMAL;

    // ERROR_CANCELLED: the user said No in the UAC prompt.
    if (!ShellExecuteExW(&sei) && GetLastError() != ERROR_CANCELLED)
    {
        StringCchPrintfW(text, ARRAYSIZE(text), L"Command is not started.\n%s", path);
        MessageBoxW(p_Window, text, L"Warning", MB_OK | MB_ICONWARNING);
    }
}

VOID AND5_ExecItem(_In_ HWND p_Window, _In_ PAND5_TB_ITEM p_Item)
{
    switch (p_Item->Kind)
    {
    case AND5_TbCommand:
        AND5_ExecCommand(p_Window, p_Item->Command);
        break;
    case AND5_TbAbout:
        MessageBoxW(p_Window, AND5_APP_ABOUT_TEXT, L"About", MB_OK | MB_ICONINFORMATION);
        break;
    case AND5_TbExit:
        DestroyWindow(p_Window);
        break;
    default:
        break;
    }
}

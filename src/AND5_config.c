#include "app.h"
#include <pathcch.h>

APP_CONFIG AppConfig;

static BOOL CfgGetEnv(_In_ PCWSTR Name, _Out_writes_(Cch) PWSTR Buffer, _In_ ULONG Cch)
{
    DWORD length = GetEnvironmentVariableW(Name, Buffer, Cch);

    if (!length || length >= Cch)
    {
        Buffer[0] = UNICODE_NULL;
        return FALSE;
    }

    return TRUE;
}

static BOOL CfgJoin(_In_ PCWSTR Base, _In_ PCWSTR Relative, _Out_writes_(Cch) PWSTR Path, _In_ ULONG Cch)
{
    return SUCCEEDED(PathCchCombine(Path, Cch, Base, Relative));
}

BOOL CfgInit(_In_opt_ PCWSTR XmlFile)
{
    CfgGetEnv(L"AND5_SCRIPTS_ROOT", AppConfig.Root, MAX_PATH);
    CfgGetEnv(L"AND5_SCRIPTS_HOME", AppConfig.Home, MAX_PATH);

    if (XmlFile && XmlFile[0])
    {
        if (FAILED(StringCchCopyW(AppConfig.XmlFile, MAX_PATH, XmlFile)))
            return FALSE;

        // Without AND5_SCRIPTS_HOME the XML folder is the home.
        if (!AppConfig.Home[0])
        {
            StringCchCopyW(AppConfig.Home, MAX_PATH, XmlFile);
            PathCchRemoveFileSpec(AppConfig.Home, MAX_PATH);
        }
    }
    else
    {
        if (!AppConfig.Home[0])
            return FALSE;

        if (!CfgJoin(AppConfig.Home, APP_XML_NAME, AppConfig.XmlFile, MAX_PATH))
            return FALSE;
    }

    // Without AND5_SCRIPTS_ROOT the home is the root. The variable is also set
    // for this process, so "AND5_SCRIPTS_ROOT#path" commands and scripts find it.
    if (!AppConfig.Root[0])
    {
        StringCchCopyW(AppConfig.Root, MAX_PATH, AppConfig.Home);
        SetEnvironmentVariableW(L"AND5_SCRIPTS_ROOT", AppConfig.Root);
    }

    return TRUE;
}

BOOL CfgResolveIcon(_In_ PCWSTR Icon, _Out_writes_(Cch) PWSTR Path, _In_ ULONG Cch)
{
    if (!Icon[0])
        return FALSE;

    return CfgJoin(AppConfig.Root, Icon, Path, Cch);
}

// "VAR#relative\path" -> %VAR%\relative\path
BOOL CfgResolveCommand(_In_ PCWSTR Command, _Out_writes_(Cch) PWSTR Path, _In_ ULONG Cch)
{
    PCWSTR relative = wcschr(Command, L'#');
    WCHAR name[64];
    WCHAR base[MAX_PATH];

    if (!relative)
        return CfgJoin(AppConfig.Root, Command, Path, Cch);

    if (FAILED(StringCchCopyNW(name, ARRAYSIZE(name), Command, relative - Command)))
        return FALSE;

    if (!CfgGetEnv(name, base, MAX_PATH))
        return FALSE;

    return CfgJoin(base, relative + 1, Path, Cch);
}

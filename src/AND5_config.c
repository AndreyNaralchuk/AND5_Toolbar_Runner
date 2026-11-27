#include "AND5_app.h"
#include <pathcch.h>

AND5_CONFIG gl_AND5_AppConfig;

static BOOL AND5_CfgGetEnv(_In_ PCWSTR p_Name, _Out_writes_(p_Cch) PWSTR p_Buffer, _In_ ULONG p_Cch)
{
    DWORD length = GetEnvironmentVariableW(p_Name, p_Buffer, p_Cch);

    if (!length || length >= p_Cch)
    {
        p_Buffer[0] = UNICODE_NULL;
        return FALSE;
    }

    return TRUE;
}

static BOOL AND5_CfgJoin(_In_ PCWSTR p_Base, _In_ PCWSTR p_Relative, _Out_writes_(p_Cch) PWSTR p_Path, _In_ ULONG p_Cch)
{
    return SUCCEEDED(PathCchCombine(p_Path, p_Cch, p_Base, p_Relative));
}

BOOL AND5_CfgInit(_In_opt_ PCWSTR p_XmlFile)
{
    AND5_CfgGetEnv(L"AND5_SCRIPTS_ROOT", gl_AND5_AppConfig.Root, MAX_PATH);
    AND5_CfgGetEnv(L"AND5_SCRIPTS_HOME", gl_AND5_AppConfig.Home, MAX_PATH);

    if (p_XmlFile && p_XmlFile[0])
    {
        if (FAILED(StringCchCopyW(gl_AND5_AppConfig.XmlFile, MAX_PATH, p_XmlFile)))
            return FALSE;

        // Without AND5_SCRIPTS_HOME the XML folder is the home.
        if (!gl_AND5_AppConfig.Home[0])
        {
            StringCchCopyW(gl_AND5_AppConfig.Home, MAX_PATH, p_XmlFile);
            PathCchRemoveFileSpec(gl_AND5_AppConfig.Home, MAX_PATH);
        }
    }
    else
    {
        if (!gl_AND5_AppConfig.Home[0])
            return FALSE;

        if (!AND5_CfgJoin(gl_AND5_AppConfig.Home, AND5_APP_XML_NAME, gl_AND5_AppConfig.XmlFile, MAX_PATH))
            return FALSE;
    }

    // Without AND5_SCRIPTS_ROOT the home is the root. The variable is also set
    // for this process, so "AND5_SCRIPTS_ROOT#path" commands and scripts find it.
    if (!gl_AND5_AppConfig.Root[0])
    {
        StringCchCopyW(gl_AND5_AppConfig.Root, MAX_PATH, gl_AND5_AppConfig.Home);
        SetEnvironmentVariableW(L"AND5_SCRIPTS_ROOT", gl_AND5_AppConfig.Root);
    }

    return TRUE;
}

BOOL AND5_CfgResolveIcon(_In_ PCWSTR p_Icon, _Out_writes_(p_Cch) PWSTR p_Path, _In_ ULONG p_Cch)
{
    if (!p_Icon[0])
        return FALSE;

    return AND5_CfgJoin(gl_AND5_AppConfig.Root, p_Icon, p_Path, p_Cch);
}

// "VAR#relative\path" -> %VAR%\relative\path
BOOL AND5_CfgResolveCommand(_In_ PCWSTR p_Command, _Out_writes_(p_Cch) PWSTR p_Path, _In_ ULONG p_Cch)
{
    PCWSTR relative = wcschr(p_Command, L'#');
    WCHAR name[64];
    WCHAR base[MAX_PATH];

    if (!relative)
        return AND5_CfgJoin(gl_AND5_AppConfig.Root, p_Command, p_Path, p_Cch);

    if (FAILED(StringCchCopyNW(name, ARRAYSIZE(name), p_Command, relative - p_Command)))
        return FALSE;

    if (!AND5_CfgGetEnv(name, base, MAX_PATH))
        return FALSE;

    return AND5_CfgJoin(base, relative + 1, p_Path, p_Cch);
}

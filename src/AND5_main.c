#include "AND5_app.h"

static UINT gl_AppTaskbarCreated;
static BOOL gl_AppConfigured;
static HICON gl_AppIcon;
static HMENU gl_AppMenu;
static AND5_TB_ITEM gl_AppRoot;

static HICON AND5_AppLoadIcon(_In_ HINSTANCE p_Instance)
{
    WCHAR path[MAX_PATH];
    HICON icon = NULL;

    if (AND5_CfgResolveIcon(AND5_APP_TRAY_ICON, path, MAX_PATH))
        icon = LoadImageW(NULL, path, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE);

    if (!icon)
        icon = LoadImageW(p_Instance, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);

    return icon;
}

static VOID AND5_AppUnload(VOID)
{
    AND5_MenuDestroy(gl_AppMenu, &gl_AppRoot);
    AND5_XmlFree(&gl_AppRoot);
    gl_AppMenu = NULL;
}

static VOID AND5_AppLoad(_In_ HWND p_Window)
{
    BOOL loaded;

    AND5_AppUnload();

    // On failure AND5_XmlLoad still gives About and Exit, so the user can close the program.
    loaded = AND5_XmlLoad(gl_AND5_AppConfig.XmlFile, &gl_AppRoot);

    if (!gl_AppConfigured)
        MessageBoxW(p_Window, L"Set AND5_SCRIPTS_HOME or give the XML file as the first argument.", L"XML file is not found", MB_OK | MB_ICONWARNING);
    else if (!loaded)
        MessageBoxW(p_Window, gl_AND5_AppConfig.XmlFile, L"XML file is not loaded", MB_OK | MB_ICONWARNING);

    gl_AppMenu = AND5_MenuBuild(&gl_AppRoot);
}

static LRESULT CALLBACK AND5_AppWndProc(_In_ HWND p_Window, _In_ UINT p_Message, _In_ WPARAM p_wParam, _In_ LPARAM p_lParam)
{
    switch (p_Message)
    {
    case WM_CREATE:
        gl_AppTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
        AND5_AppLoad(p_Window);
        AND5_TrayAdd(p_Window, gl_AppIcon);
        return 0;
    case AND5_WM_TRAY:
        if (p_lParam == WM_LBUTTONUP || p_lParam == WM_RBUTTONUP)
            AND5_MenuShow(p_Window, gl_AppMenu);
        return 0;
    case WM_COMMAND:
        {
            PAND5_TB_ITEM item = AND5_MenuFind(&gl_AppRoot, LOWORD(p_wParam));

            if (item)
                AND5_ExecItem(p_Window, item);
        }
        return 0;
    case WM_DESTROY:
        AND5_TrayRemove(p_Window);
        AND5_AppUnload();
        PostQuitMessage(0);
        return 0;
    }

    if (gl_AppTaskbarCreated && p_Message == gl_AppTaskbarCreated)
    {
        AND5_TrayAdd(p_Window, gl_AppIcon);
        return 0;
    }

    return DefWindowProcW(p_Window, p_Message, p_wParam, p_lParam);
}

int WINAPI wWinMain(_In_ HINSTANCE p_Instance, _In_opt_ HINSTANCE p_PrevInstance, _In_ PWSTR p_CmdLine, _In_ int p_CmdShow)
{
    WNDCLASSEXW wc = { sizeof(wc) };
    PWSTR *argv;
    int argc;
    MSG msg;

    UNREFERENCED_PARAMETER(p_PrevInstance);
    UNREFERENCED_PARAMETER(p_CmdLine);
    UNREFERENCED_PARAMETER(p_CmdShow);

    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    gl_AppConfigured = AND5_CfgInit(argv && argc > 1 ? argv[1] : NULL);
    LocalFree(argv);

    gl_AppIcon = AND5_AppLoadIcon(p_Instance);

    wc.lpfnWndProc = AND5_AppWndProc;
    wc.hInstance = p_Instance;
    wc.hIcon = gl_AppIcon;
    wc.lpszClassName = AND5_APP_NAME;

    if (!RegisterClassExW(&wc))
        return 1;

    if (!CreateWindowExW(0, AND5_APP_NAME, AND5_APP_NAME, WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, p_Instance, NULL))
        return 1;

    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DestroyIcon(gl_AppIcon);
    return (int)msg.wParam;
}

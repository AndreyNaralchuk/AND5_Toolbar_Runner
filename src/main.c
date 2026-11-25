#include "app.h"

static UINT AppTaskbarCreated;
static BOOL AppConfigured;
static HICON AppIcon;
static HMENU AppMenu;
static TB_ITEM AppRoot;

static HICON AppLoadIcon(_In_ HINSTANCE Instance)
{
    WCHAR path[MAX_PATH];
    HICON icon = NULL;

    if (CfgResolveIcon(APP_TRAY_ICON, path, MAX_PATH))
        icon = LoadImageW(NULL, path, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE);

    if (!icon)
        icon = LoadImageW(Instance, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);

    return icon;
}

static VOID AppUnload(VOID)
{
    MenuDestroy(AppMenu, &AppRoot);
    XmlFree(&AppRoot);
    AppMenu = NULL;
}

static VOID AppLoad(_In_ HWND Window)
{
    BOOL loaded;

    AppUnload();

    // On failure XmlLoad still gives About and Exit, so the user can close the program.
    loaded = XmlLoad(AppConfig.XmlFile, &AppRoot);

    if (!AppConfigured)
        MessageBoxW(Window, L"Set AND5_SCRIPTS_HOME or give the XML file as the first argument.", L"XML file is not found", MB_OK | MB_ICONWARNING);
    else if (!loaded)
        MessageBoxW(Window, AppConfig.XmlFile, L"XML file is not loaded", MB_OK | MB_ICONWARNING);

    AppMenu = MenuBuild(&AppRoot);
}

static LRESULT CALLBACK AppWndProc(_In_ HWND Window, _In_ UINT Message, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    switch (Message)
    {
    case WM_CREATE:
        AppTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
        AppLoad(Window);
        TrayAdd(Window, AppIcon);
        return 0;
    case WM_TRAY:
        if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP)
            MenuShow(Window, AppMenu);
        return 0;
    case WM_COMMAND:
        {
            PTB_ITEM item = MenuFind(&AppRoot, LOWORD(wParam));

            if (item)
                ExecItem(Window, item);
        }
        return 0;
    case WM_DESTROY:
        TrayRemove(Window);
        AppUnload();
        PostQuitMessage(0);
        return 0;
    }

    if (AppTaskbarCreated && Message == AppTaskbarCreated)
    {
        TrayAdd(Window, AppIcon);
        return 0;
    }

    return DefWindowProcW(Window, Message, wParam, lParam);
}

int WINAPI wWinMain(_In_ HINSTANCE Instance, _In_opt_ HINSTANCE PrevInstance, _In_ PWSTR CmdLine, _In_ int CmdShow)
{
    WNDCLASSEXW wc = { sizeof(wc) };
    PWSTR *argv;
    int argc;
    MSG msg;

    UNREFERENCED_PARAMETER(PrevInstance);
    UNREFERENCED_PARAMETER(CmdLine);
    UNREFERENCED_PARAMETER(CmdShow);

    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    AppConfigured = CfgInit(argv && argc > 1 ? argv[1] : NULL);
    LocalFree(argv);

    AppIcon = AppLoadIcon(Instance);

    wc.lpfnWndProc = AppWndProc;
    wc.hInstance = Instance;
    wc.hIcon = AppIcon;
    wc.lpszClassName = APP_NAME;

    if (!RegisterClassExW(&wc))
        return 1;

    if (!CreateWindowExW(0, APP_NAME, APP_NAME, WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, Instance, NULL))
        return 1;

    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DestroyIcon(AppIcon);
    return (int)msg.wParam;
}

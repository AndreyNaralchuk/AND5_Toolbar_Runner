#include "app.h"

static UINT MenuNextId;

static VOID MenuLoadIcon(_Inout_ PTB_ITEM Item)
{
    WCHAR path[MAX_PATH];

    Item->Bitmap = NULL;

    if (CfgResolveIcon(Item->Icon, path, MAX_PATH))
        Item->Bitmap = IcoLoadBitmap(path, GetSystemMetrics(SM_CXSMICON));
}

static VOID MenuInsert(_In_ HMENU Menu, _In_ MENUITEMINFOW *Info)
{
    InsertMenuItemW(Menu, GetMenuItemCount(Menu), TRUE, Info);
}

static VOID MenuAddLeaf(_In_ HMENU Menu, _Inout_ PTB_ITEM Item)
{
    MENUITEMINFOW mii = { sizeof(mii) };

    Item->Id = MenuNextId++;
    mii.fMask = MIIM_STRING | MIIM_ID | MIIM_BITMAP;
    mii.wID = Item->Id;
    mii.dwTypeData = Item->Name;
    mii.hbmpItem = Item->Bitmap;
    MenuInsert(Menu, &mii);
}

static VOID MenuAdd(_In_ HMENU Menu, _Inout_ PTB_ITEM Item)
{
    MENUITEMINFOW mii = { sizeof(mii) };
    HMENU subMenu;

    if (Item->Kind == TbSeparator)
    {
        AppendMenuW(Menu, MF_SEPARATOR, 0, NULL);
        return;
    }

    MenuLoadIcon(Item);

    if (!Item->Count)
    {
        MenuAddLeaf(Menu, Item);
        return;
    }

    subMenu = CreatePopupMenu();

    // A popup item cannot be clicked, so its own command goes first in the submenu.
    if (Item->Kind == TbCommand)
    {
        MenuAddLeaf(subMenu, Item);
        AppendMenuW(subMenu, MF_SEPARATOR, 0, NULL);
    }

    for (ULONG i = 0; i < Item->Count; i++)
        MenuAdd(subMenu, &Item->Items[i]);

    mii.fMask = MIIM_STRING | MIIM_SUBMENU | MIIM_BITMAP;
    mii.hSubMenu = subMenu;
    mii.dwTypeData = Item->Name;
    mii.hbmpItem = Item->Bitmap;
    MenuInsert(Menu, &mii);
}

HMENU MenuBuild(_Inout_ PTB_ITEM Root)
{
    HMENU menu = CreatePopupMenu();

    MenuNextId = ID_FIRST;

    for (ULONG i = 0; i < Root->Count; i++)
        MenuAdd(menu, &Root->Items[i]);

    return menu;
}

static VOID MenuFreeBitmaps(_Inout_ PTB_ITEM Item)
{
    if (Item->Bitmap)
        DeleteObject(Item->Bitmap);

    Item->Bitmap = NULL;

    for (ULONG i = 0; i < Item->Count; i++)
        MenuFreeBitmaps(&Item->Items[i]);
}

VOID MenuDestroy(_In_opt_ HMENU Menu, _Inout_ PTB_ITEM Root)
{
    if (Menu)
        DestroyMenu(Menu);

    MenuFreeBitmaps(Root);
}

PTB_ITEM MenuFind(_In_ PTB_ITEM Root, _In_ UINT Id)
{
    PTB_ITEM item;

    for (ULONG i = 0; i < Root->Count; i++)
    {
        if (Root->Items[i].Id == Id)
            return &Root->Items[i];

        item = MenuFind(&Root->Items[i], Id);

        if (item)
            return item;
    }

    return NULL;
}

VOID MenuShow(_In_ HWND Window, _In_ HMENU Menu)
{
    POINT point;

    GetCursorPos(&point);
    SetForegroundWindow(Window);
    TrackPopupMenuEx(Menu, TPM_RIGHTBUTTON, point.x, point.y, Window, NULL);
    PostMessageW(Window, WM_NULL, 0, 0);
}

#include "AND5_app.h"

static UINT gl_MenuNextId;

static VOID AND5_MenuLoadIcon(_Inout_ PAND5_TB_ITEM p_Item)
{
    WCHAR path[MAX_PATH];

    p_Item->Bitmap = NULL;

    if (AND5_CfgResolveIcon(p_Item->Icon, path, MAX_PATH))
        p_Item->Bitmap = AND5_IcoLoadBitmap(path, GetSystemMetrics(SM_CXSMICON));
}

static VOID AND5_MenuInsert(_In_ HMENU p_Menu, _In_ MENUITEMINFOW *p_Info)
{
    InsertMenuItemW(p_Menu, GetMenuItemCount(p_Menu), TRUE, p_Info);
}

static VOID AND5_MenuAddLeaf(_In_ HMENU p_Menu, _Inout_ PAND5_TB_ITEM p_Item)
{
    MENUITEMINFOW mii = { sizeof(mii) };

    p_Item->Id = gl_MenuNextId++;
    mii.fMask = MIIM_STRING | MIIM_ID | MIIM_BITMAP;
    mii.wID = p_Item->Id;
    mii.dwTypeData = p_Item->Name;
    mii.hbmpItem = p_Item->Bitmap;
    AND5_MenuInsert(p_Menu, &mii);
}

static VOID AND5_MenuAdd(_In_ HMENU p_Menu, _Inout_ PAND5_TB_ITEM p_Item)
{
    MENUITEMINFOW mii = { sizeof(mii) };
    HMENU subMenu;

    if (p_Item->Kind == AND5_TbSeparator)
    {
        AppendMenuW(p_Menu, MF_SEPARATOR, 0, NULL);
        return;
    }

    AND5_MenuLoadIcon(p_Item);

    if (!p_Item->Count)
    {
        AND5_MenuAddLeaf(p_Menu, p_Item);
        return;
    }

    subMenu = CreatePopupMenu();

    // A popup item cannot be clicked, so its own command goes first in the submenu.
    if (p_Item->Kind == AND5_TbCommand)
    {
        AND5_MenuAddLeaf(subMenu, p_Item);
        AppendMenuW(subMenu, MF_SEPARATOR, 0, NULL);
    }

    for (ULONG i = 0; i < p_Item->Count; i++)
        AND5_MenuAdd(subMenu, &p_Item->Items[i]);

    mii.fMask = MIIM_STRING | MIIM_SUBMENU | MIIM_BITMAP;
    mii.hSubMenu = subMenu;
    mii.dwTypeData = p_Item->Name;
    mii.hbmpItem = p_Item->Bitmap;
    AND5_MenuInsert(p_Menu, &mii);
}

HMENU AND5_MenuBuild(_Inout_ PAND5_TB_ITEM p_Root)
{
    HMENU menu = CreatePopupMenu();

    gl_MenuNextId = AND5_ID_FIRST;

    for (ULONG i = 0; i < p_Root->Count; i++)
        AND5_MenuAdd(menu, &p_Root->Items[i]);

    return menu;
}

static VOID AND5_MenuFreeBitmaps(_Inout_ PAND5_TB_ITEM p_Item)
{
    if (p_Item->Bitmap)
        DeleteObject(p_Item->Bitmap);

    p_Item->Bitmap = NULL;

    for (ULONG i = 0; i < p_Item->Count; i++)
        AND5_MenuFreeBitmaps(&p_Item->Items[i]);
}

VOID AND5_MenuDestroy(_In_opt_ HMENU p_Menu, _Inout_ PAND5_TB_ITEM p_Root)
{
    if (p_Menu)
        DestroyMenu(p_Menu);

    AND5_MenuFreeBitmaps(p_Root);
}

PAND5_TB_ITEM AND5_MenuFind(_In_ PAND5_TB_ITEM p_Root, _In_ UINT p_Id)
{
    PAND5_TB_ITEM item;

    for (ULONG i = 0; i < p_Root->Count; i++)
    {
        if (p_Root->Items[i].Id == p_Id)
            return &p_Root->Items[i];

        item = AND5_MenuFind(&p_Root->Items[i], p_Id);

        if (item)
            return item;
    }

    return NULL;
}

VOID AND5_MenuShow(_In_ HWND p_Window, _In_ HMENU p_Menu)
{
    POINT point;

    GetCursorPos(&point);
    SetForegroundWindow(p_Window);
    TrackPopupMenuEx(p_Menu, TPM_RIGHTBUTTON, point.x, point.y, p_Window, NULL);
    PostMessageW(p_Window, WM_NULL, 0, 0);
}

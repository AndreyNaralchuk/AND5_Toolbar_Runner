#include "AND5_app.h"
#include <uxtheme.h>

// Port of PhIconToBitmap (phlib\icotobmp.c), based on the MSDN sample bb757020.

static HBITMAP AND5_IcoCreateBitmap(_In_ HDC p_Dc, _In_ LONG p_Width, _In_ LONG p_Height)
{
    BITMAPINFO bmi = { 0 };
    PVOID bits;

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = p_Width;
    bmi.bmiHeader.biHeight = p_Height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    return CreateDIBSection(p_Dc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
}

static BOOL AND5_IcoHasAlpha(_In_ PULONG p_Argb, _In_ LONG p_Width, _In_ LONG p_Height, _In_ LONG p_RowWidth)
{
    for (LONG y = 0; y < p_Height; y++)
    {
        for (LONG x = 0; x < p_Width; x++)
        {
            if (p_Argb[x] & 0xff000000)
                return TRUE;
        }

        p_Argb += p_RowWidth;
    }

    return FALSE;
}

// An icon without an alpha channel: the AND mask says which pixels are transparent.
static VOID AND5_IcoMaskToAlpha(_In_ HDC p_Dc, _Inout_ PULONG p_Argb, _In_ HBITMAP p_Mask, _In_ LONG p_Width, _In_ LONG p_Height, _In_ LONG p_RowWidth)
{
    BITMAPINFO bmi = { 0 };
    PULONG mask;
    PULONG bits;

    bits = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)p_Width * p_Height * sizeof(ULONG));

    if (!bits)
        return;

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = p_Width;
    bmi.bmiHeader.biHeight = p_Height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    if (GetDIBits(p_Dc, p_Mask, 0, p_Height, bits, &bmi, DIB_RGB_COLORS) == p_Height)
    {
        mask = bits;

        for (LONG y = 0; y < p_Height; y++)
        {
            for (LONG x = 0; x < p_Width; x++)
                p_Argb[x] = mask[x] ? 0 : p_Argb[x] | 0xff000000;

            mask += p_Width;
            p_Argb += p_RowWidth;
        }
    }

    HeapFree(GetProcessHeap(), 0, bits);
}

static VOID AND5_IcoFixAlpha(_In_ HPAINTBUFFER p_Buffer, _In_ HDC p_Dc, _In_ HICON p_Icon, _In_ LONG p_Width, _In_ LONG p_Height)
{
    ICONINFO info;
    RGBQUAD *quad;
    int rowWidth;

    if (FAILED(GetBufferedPaintBits(p_Buffer, &quad, &rowWidth)))
        return;

    if (AND5_IcoHasAlpha((PULONG)quad, p_Width, p_Height, rowWidth))
        return;

    if (!GetIconInfo(p_Icon, &info))
        return;

    if (info.hbmMask)
    {
        AND5_IcoMaskToAlpha(p_Dc, (PULONG)quad, info.hbmMask, p_Width, p_Height, rowWidth);
        DeleteObject(info.hbmMask);
    }

    if (info.hbmColor)
        DeleteObject(info.hbmColor);
}

static HBITMAP AND5_IcoToBitmap(_In_ HICON p_Icon, _In_ LONG p_Size)
{
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    BP_PAINTPARAMS params = { sizeof(params) };
    RECT rect = { 0, 0, p_Size, p_Size };
    HPAINTBUFFER buffer;
    HBITMAP bitmap;
    HBITMAP oldBitmap;
    HDC bufferDc;
    HDC dc;

    dc = CreateCompatibleDC(NULL);

    if (!dc)
        return NULL;

    bitmap = AND5_IcoCreateBitmap(dc, p_Size, p_Size);

    if (!bitmap)
    {
        DeleteDC(dc);
        return NULL;
    }

    oldBitmap = SelectObject(dc, bitmap);
    params.dwFlags = BPPF_ERASE;
    params.pBlendFunction = &blend;
    buffer = BeginBufferedPaint(dc, &rect, BPBF_DIB, &params, &bufferDc);

    if (buffer)
    {
        DrawIconEx(bufferDc, 0, 0, p_Icon, p_Size, p_Size, 0, NULL, DI_NORMAL);
        AND5_IcoFixAlpha(buffer, dc, p_Icon, p_Size, p_Size);
        EndBufferedPaint(buffer, TRUE);
    }
    else
    {
        FillRect(dc, &rect, (HBRUSH)(COLOR_MENU + 1));
        DrawIconEx(dc, 0, 0, p_Icon, p_Size, p_Size, 0, NULL, DI_NORMAL);
    }

    SelectObject(dc, oldBitmap);
    DeleteDC(dc);

    return bitmap;
}

HBITMAP AND5_IcoLoadBitmap(_In_ PCWSTR p_Path, _In_ LONG p_Size)
{
    HBITMAP bitmap;
    HICON icon;

    icon = LoadImageW(NULL, p_Path, IMAGE_ICON, p_Size, p_Size, LR_LOADFROMFILE);

    if (!icon)
        return NULL;

    bitmap = AND5_IcoToBitmap(icon, p_Size);
    DestroyIcon(icon);

    return bitmap;
}

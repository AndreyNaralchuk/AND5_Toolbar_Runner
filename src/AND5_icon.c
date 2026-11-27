#include "app.h"
#include <uxtheme.h>

// Port of PhIconToBitmap (phlib\icotobmp.c), based on the MSDN sample bb757020.

static HBITMAP IcoCreateBitmap(_In_ HDC Dc, _In_ LONG Width, _In_ LONG Height)
{
    BITMAPINFO bmi = { 0 };
    PVOID bits;

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = Width;
    bmi.bmiHeader.biHeight = Height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    return CreateDIBSection(Dc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
}

static BOOL IcoHasAlpha(_In_ PULONG Argb, _In_ LONG Width, _In_ LONG Height, _In_ LONG RowWidth)
{
    for (LONG y = 0; y < Height; y++)
    {
        for (LONG x = 0; x < Width; x++)
        {
            if (Argb[x] & 0xff000000)
                return TRUE;
        }

        Argb += RowWidth;
    }

    return FALSE;
}

// An icon without an alpha channel: the AND mask says which pixels are transparent.
static VOID IcoMaskToAlpha(_In_ HDC Dc, _Inout_ PULONG Argb, _In_ HBITMAP Mask, _In_ LONG Width, _In_ LONG Height, _In_ LONG RowWidth)
{
    BITMAPINFO bmi = { 0 };
    PULONG mask;
    PULONG bits;

    bits = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)Width * Height * sizeof(ULONG));

    if (!bits)
        return;

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = Width;
    bmi.bmiHeader.biHeight = Height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    if (GetDIBits(Dc, Mask, 0, Height, bits, &bmi, DIB_RGB_COLORS) == Height)
    {
        mask = bits;

        for (LONG y = 0; y < Height; y++)
        {
            for (LONG x = 0; x < Width; x++)
                Argb[x] = mask[x] ? 0 : Argb[x] | 0xff000000;

            mask += Width;
            Argb += RowWidth;
        }
    }

    HeapFree(GetProcessHeap(), 0, bits);
}

static VOID IcoFixAlpha(_In_ HPAINTBUFFER Buffer, _In_ HDC Dc, _In_ HICON Icon, _In_ LONG Width, _In_ LONG Height)
{
    ICONINFO info;
    RGBQUAD *quad;
    int rowWidth;

    if (FAILED(GetBufferedPaintBits(Buffer, &quad, &rowWidth)))
        return;

    if (IcoHasAlpha((PULONG)quad, Width, Height, rowWidth))
        return;

    if (!GetIconInfo(Icon, &info))
        return;

    if (info.hbmMask)
    {
        IcoMaskToAlpha(Dc, (PULONG)quad, info.hbmMask, Width, Height, rowWidth);
        DeleteObject(info.hbmMask);
    }

    if (info.hbmColor)
        DeleteObject(info.hbmColor);
}

static HBITMAP IcoToBitmap(_In_ HICON Icon, _In_ LONG Size)
{
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    BP_PAINTPARAMS params = { sizeof(params) };
    RECT rect = { 0, 0, Size, Size };
    HPAINTBUFFER buffer;
    HBITMAP bitmap;
    HBITMAP oldBitmap;
    HDC bufferDc;
    HDC dc;

    dc = CreateCompatibleDC(NULL);

    if (!dc)
        return NULL;

    bitmap = IcoCreateBitmap(dc, Size, Size);

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
        DrawIconEx(bufferDc, 0, 0, Icon, Size, Size, 0, NULL, DI_NORMAL);
        IcoFixAlpha(buffer, dc, Icon, Size, Size);
        EndBufferedPaint(buffer, TRUE);
    }
    else
    {
        FillRect(dc, &rect, (HBRUSH)(COLOR_MENU + 1));
        DrawIconEx(dc, 0, 0, Icon, Size, Size, 0, NULL, DI_NORMAL);
    }

    SelectObject(dc, oldBitmap);
    DeleteDC(dc);

    return bitmap;
}

HBITMAP IcoLoadBitmap(_In_ PCWSTR Path, _In_ LONG Size)
{
    HBITMAP bitmap;
    HICON icon;

    icon = LoadImageW(NULL, Path, IMAGE_ICON, Size, Size, LR_LOADFROMFILE);

    if (!icon)
        return NULL;

    bitmap = IcoToBitmap(icon, Size);
    DestroyIcon(icon);

    return bitmap;
}

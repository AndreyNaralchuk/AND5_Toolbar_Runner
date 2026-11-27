#include "AND5_app.h"
#include <objbase.h>
#include <initguid.h>
#include <xmllite.h>
#include <shlwapi.h>

typedef enum _AND5_XML_FIELD
{
    AND5_FieldNone,
    AND5_FieldName,
    AND5_FieldIcon,
    AND5_FieldCommand
} AND5_XML_FIELD;

typedef struct _AND5_XML_CONTEXT
{
    PAND5_TB_ITEM Root;
    AND5_TB_ITEM Menu;       // <menu> being read; its children are <submenu> items
    AND5_TB_ITEM Sub;        // <submenu> being read
    BOOL InMenu;
    BOOL InSub;
    AND5_XML_FIELD Field;
} AND5_XML_CONTEXT;

// Items are copied by value, so a parent never keeps pointers into its own array.
static BOOL AND5_XmlAppend(_Inout_ PAND5_TB_ITEM p_Parent, _In_ const AND5_TB_ITEM *p_Item)
{
    PAND5_TB_ITEM items;

    if (p_Parent->Items)
        items = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, p_Parent->Items, (p_Parent->Count + 1) * sizeof(AND5_TB_ITEM));
    else
        items = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(AND5_TB_ITEM));

    if (!items)
        return FALSE;

    p_Parent->Items = items;
    items[p_Parent->Count++] = *p_Item;
    return TRUE;
}

static VOID AND5_XmlClassify(_Inout_ PAND5_TB_ITEM p_Item)
{
    if (!_wcsicmp(p_Item->Name, L"Separator"))
        p_Item->Kind = AND5_TbSeparator;
    else if (p_Item->Command[0])
        p_Item->Kind = AND5_TbCommand;
    else if (!_wcsicmp(p_Item->Name, L"About"))
        p_Item->Kind = AND5_TbAbout;
    else if (!_wcsicmp(p_Item->Name, L"Exit"))
        p_Item->Kind = AND5_TbExit;
    else
        p_Item->Kind = AND5_TbNone;
}

static VOID AND5_XmlSetField(_Inout_ AND5_XML_CONTEXT *p_Context, _In_ PCWSTR p_Text)
{
    PAND5_TB_ITEM item = p_Context->InSub ? &p_Context->Sub : &p_Context->Menu;

    switch (p_Context->Field)
    {
    case AND5_FieldName:
        StringCchCopyW(item->Name, ARRAYSIZE(item->Name), p_Text);
        break;
    case AND5_FieldIcon:
        StringCchCopyW(item->Icon, ARRAYSIZE(item->Icon), p_Text);
        break;
    case AND5_FieldCommand:
        StringCchCopyW(item->Command, ARRAYSIZE(item->Command), p_Text);
        break;
    }
}

static VOID AND5_XmlOnStart(_Inout_ AND5_XML_CONTEXT *p_Context, _In_ PCWSTR p_Name)
{
    if (!p_Context->InMenu)
    {
        if (!wcscmp(p_Name, L"menu"))
        {
            ZeroMemory(&p_Context->Menu, sizeof(AND5_TB_ITEM));
            p_Context->InMenu = TRUE;
        }

        return;
    }

    if (!p_Context->InSub && !wcscmp(p_Name, L"submenu"))
    {
        ZeroMemory(&p_Context->Sub, sizeof(AND5_TB_ITEM));
        p_Context->InSub = TRUE;
    }
    else if (!wcscmp(p_Name, L"name"))
        p_Context->Field = AND5_FieldName;
    else if (!wcscmp(p_Name, L"icon"))
        p_Context->Field = AND5_FieldIcon;
    else if (!wcscmp(p_Name, L"command"))
        p_Context->Field = AND5_FieldCommand;
    else
        p_Context->Field = AND5_FieldNone;
}

static BOOL AND5_XmlOnEnd(_Inout_ AND5_XML_CONTEXT *p_Context, _In_ PCWSTR p_Name)
{
    BOOL ok = TRUE;

    if (p_Context->InSub && !wcscmp(p_Name, L"submenu"))
    {
        AND5_XmlClassify(&p_Context->Sub);
        ok = AND5_XmlAppend(&p_Context->Menu, &p_Context->Sub);
        p_Context->InSub = FALSE;
        p_Context->Field = AND5_FieldNone;
    }
    else if (p_Context->InMenu && !p_Context->InSub && !wcscmp(p_Name, L"menu"))
    {
        AND5_XmlClassify(&p_Context->Menu);
        ok = AND5_XmlAppend(p_Context->Root, &p_Context->Menu);

        if (ok)
            ZeroMemory(&p_Context->Menu, sizeof(AND5_TB_ITEM)); // children now belong to Root

        p_Context->InMenu = FALSE;
        p_Context->Field = AND5_FieldNone;
    }
    else
        p_Context->Field = AND5_FieldNone;

    return ok;
}

static HRESULT AND5_XmlParse(_In_ IXmlReader *p_Reader, _Inout_ AND5_XML_CONTEXT *p_Context)
{
    XmlNodeType type;
    PCWSTR text;
    HRESULT hr;

    while ((hr = p_Reader->lpVtbl->Read(p_Reader, &type)) == S_OK)
    {
        switch (type)
        {
        case XmlNodeType_Element:
            hr = p_Reader->lpVtbl->GetLocalName(p_Reader, &text, NULL);

            if (FAILED(hr))
                return hr;

            AND5_XmlOnStart(p_Context, text);

            // An empty element such as <icon /> has no end node.
            if (p_Reader->lpVtbl->IsEmptyElement(p_Reader) && !AND5_XmlOnEnd(p_Context, text))
                return E_OUTOFMEMORY;

            break;
        case XmlNodeType_EndElement:
            hr = p_Reader->lpVtbl->GetLocalName(p_Reader, &text, NULL);

            if (FAILED(hr))
                return hr;

            if (!AND5_XmlOnEnd(p_Context, text))
                return E_OUTOFMEMORY;

            break;
        case XmlNodeType_Text:
        case XmlNodeType_CDATA:
            if (p_Context->InMenu && p_Context->Field != AND5_FieldNone)
            {
                hr = p_Reader->lpVtbl->GetValue(p_Reader, &text, NULL);

                if (FAILED(hr))
                    return hr;

                AND5_XmlSetField(p_Context, text);
            }

            break;
        }
    }

    return hr == S_FALSE ? S_OK : hr;
}

// Minimal menu, so the user can still reach About and Exit when the XML is broken.
static VOID AND5_XmlFallback(_Inout_ PAND5_TB_ITEM p_Root)
{
    AND5_TB_ITEM about = { AND5_TbAbout };
    AND5_TB_ITEM exit = { AND5_TbExit };

    StringCchCopyW(about.Name, ARRAYSIZE(about.Name), L"About");
    StringCchCopyW(exit.Name, ARRAYSIZE(exit.Name), L"Exit");
    AND5_XmlAppend(p_Root, &about);
    AND5_XmlAppend(p_Root, &exit);
}

BOOL AND5_XmlLoad(_In_ PCWSTR p_XmlFile, _Out_ PAND5_TB_ITEM p_Root)
{
    AND5_XML_CONTEXT context = { 0 };
    IStream *stream = NULL;
    IXmlReader *reader = NULL;
    HRESULT hr;

    ZeroMemory(p_Root, sizeof(AND5_TB_ITEM));
    context.Root = p_Root;

    hr = SHCreateStreamOnFileW(p_XmlFile, STGM_READ | STGM_SHARE_DENY_NONE, &stream);

    if (SUCCEEDED(hr))
        hr = CreateXmlReader(&IID_IXmlReader, (PVOID *)&reader, NULL);

    if (SUCCEEDED(hr))
        hr = reader->lpVtbl->SetProperty(reader, XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit);

    if (SUCCEEDED(hr))
        hr = reader->lpVtbl->SetInput(reader, (IUnknown *)stream);

    if (SUCCEEDED(hr))
        hr = AND5_XmlParse(reader, &context);

    if (reader)
        reader->lpVtbl->Release(reader);

    if (stream)
        stream->lpVtbl->Release(stream);

    if (FAILED(hr) || !p_Root->Count)
    {
        AND5_XmlFree(&context.Menu); // partly read <menu>
        AND5_XmlFree(p_Root);
        AND5_XmlFallback(p_Root);
        return FALSE;
    }

    return TRUE;
}

VOID AND5_XmlFree(_Inout_ PAND5_TB_ITEM p_Root)
{
    for (ULONG i = 0; i < p_Root->Count; i++)
        AND5_XmlFree(&p_Root->Items[i]);

    if (p_Root->Items)
        HeapFree(GetProcessHeap(), 0, p_Root->Items);

    p_Root->Items = NULL;
    p_Root->Count = 0;
}

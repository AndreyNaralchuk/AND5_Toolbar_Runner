#include "app.h"
#include <objbase.h>
#include <initguid.h>
#include <xmllite.h>
#include <shlwapi.h>

typedef enum _XML_FIELD
{
    FieldNone,
    FieldName,
    FieldIcon,
    FieldCommand
} XML_FIELD;

typedef struct _XML_CONTEXT
{
    PTB_ITEM Root;
    TB_ITEM Menu;       // <menu> being read; its children are <submenu> items
    TB_ITEM Sub;        // <submenu> being read
    BOOL InMenu;
    BOOL InSub;
    XML_FIELD Field;
} XML_CONTEXT;

// Items are copied by value, so a parent never keeps pointers into its own array.
static BOOL XmlAppend(_Inout_ PTB_ITEM Parent, _In_ const TB_ITEM *Item)
{
    PTB_ITEM items;

    if (Parent->Items)
        items = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Parent->Items, (Parent->Count + 1) * sizeof(TB_ITEM));
    else
        items = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TB_ITEM));

    if (!items)
        return FALSE;

    Parent->Items = items;
    items[Parent->Count++] = *Item;
    return TRUE;
}

static VOID XmlClassify(_Inout_ PTB_ITEM Item)
{
    if (!_wcsicmp(Item->Name, L"Separator"))
        Item->Kind = TbSeparator;
    else if (Item->Command[0])
        Item->Kind = TbCommand;
    else if (!_wcsicmp(Item->Name, L"About"))
        Item->Kind = TbAbout;
    else if (!_wcsicmp(Item->Name, L"Exit"))
        Item->Kind = TbExit;
    else
        Item->Kind = TbNone;
}

static VOID XmlSetField(_Inout_ XML_CONTEXT *Context, _In_ PCWSTR Text)
{
    PTB_ITEM item = Context->InSub ? &Context->Sub : &Context->Menu;

    switch (Context->Field)
    {
    case FieldName:
        StringCchCopyW(item->Name, ARRAYSIZE(item->Name), Text);
        break;
    case FieldIcon:
        StringCchCopyW(item->Icon, ARRAYSIZE(item->Icon), Text);
        break;
    case FieldCommand:
        StringCchCopyW(item->Command, ARRAYSIZE(item->Command), Text);
        break;
    }
}

static VOID XmlOnStart(_Inout_ XML_CONTEXT *Context, _In_ PCWSTR Name)
{
    if (!Context->InMenu)
    {
        if (!wcscmp(Name, L"menu"))
        {
            ZeroMemory(&Context->Menu, sizeof(TB_ITEM));
            Context->InMenu = TRUE;
        }

        return;
    }

    if (!Context->InSub && !wcscmp(Name, L"submenu"))
    {
        ZeroMemory(&Context->Sub, sizeof(TB_ITEM));
        Context->InSub = TRUE;
    }
    else if (!wcscmp(Name, L"name"))
        Context->Field = FieldName;
    else if (!wcscmp(Name, L"icon"))
        Context->Field = FieldIcon;
    else if (!wcscmp(Name, L"command"))
        Context->Field = FieldCommand;
    else
        Context->Field = FieldNone;
}

static BOOL XmlOnEnd(_Inout_ XML_CONTEXT *Context, _In_ PCWSTR Name)
{
    BOOL ok = TRUE;

    if (Context->InSub && !wcscmp(Name, L"submenu"))
    {
        XmlClassify(&Context->Sub);
        ok = XmlAppend(&Context->Menu, &Context->Sub);
        Context->InSub = FALSE;
        Context->Field = FieldNone;
    }
    else if (Context->InMenu && !Context->InSub && !wcscmp(Name, L"menu"))
    {
        XmlClassify(&Context->Menu);
        ok = XmlAppend(Context->Root, &Context->Menu);

        if (ok)
            ZeroMemory(&Context->Menu, sizeof(TB_ITEM)); // children now belong to Root

        Context->InMenu = FALSE;
        Context->Field = FieldNone;
    }
    else
        Context->Field = FieldNone;

    return ok;
}

static HRESULT XmlParse(_In_ IXmlReader *Reader, _Inout_ XML_CONTEXT *Context)
{
    XmlNodeType type;
    PCWSTR text;
    HRESULT hr;

    while ((hr = Reader->lpVtbl->Read(Reader, &type)) == S_OK)
    {
        switch (type)
        {
        case XmlNodeType_Element:
            hr = Reader->lpVtbl->GetLocalName(Reader, &text, NULL);

            if (FAILED(hr))
                return hr;

            XmlOnStart(Context, text);

            // An empty element such as <icon /> has no end node.
            if (Reader->lpVtbl->IsEmptyElement(Reader) && !XmlOnEnd(Context, text))
                return E_OUTOFMEMORY;

            break;
        case XmlNodeType_EndElement:
            hr = Reader->lpVtbl->GetLocalName(Reader, &text, NULL);

            if (FAILED(hr))
                return hr;

            if (!XmlOnEnd(Context, text))
                return E_OUTOFMEMORY;

            break;
        case XmlNodeType_Text:
        case XmlNodeType_CDATA:
            if (Context->InMenu && Context->Field != FieldNone)
            {
                hr = Reader->lpVtbl->GetValue(Reader, &text, NULL);

                if (FAILED(hr))
                    return hr;

                XmlSetField(Context, text);
            }

            break;
        }
    }

    return hr == S_FALSE ? S_OK : hr;
}

// Minimal menu, so the user can still reach About and Exit when the XML is broken.
static VOID XmlFallback(_Inout_ PTB_ITEM Root)
{
    TB_ITEM about = { TbAbout };
    TB_ITEM exit = { TbExit };

    StringCchCopyW(about.Name, ARRAYSIZE(about.Name), L"About");
    StringCchCopyW(exit.Name, ARRAYSIZE(exit.Name), L"Exit");
    XmlAppend(Root, &about);
    XmlAppend(Root, &exit);
}

BOOL XmlLoad(_In_ PCWSTR XmlFile, _Out_ PTB_ITEM Root)
{
    XML_CONTEXT context = { 0 };
    IStream *stream = NULL;
    IXmlReader *reader = NULL;
    HRESULT hr;

    ZeroMemory(Root, sizeof(TB_ITEM));
    context.Root = Root;

    hr = SHCreateStreamOnFileW(XmlFile, STGM_READ | STGM_SHARE_DENY_NONE, &stream);

    if (SUCCEEDED(hr))
        hr = CreateXmlReader(&IID_IXmlReader, (PVOID *)&reader, NULL);

    if (SUCCEEDED(hr))
        hr = reader->lpVtbl->SetProperty(reader, XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit);

    if (SUCCEEDED(hr))
        hr = reader->lpVtbl->SetInput(reader, (IUnknown *)stream);

    if (SUCCEEDED(hr))
        hr = XmlParse(reader, &context);

    if (reader)
        reader->lpVtbl->Release(reader);

    if (stream)
        stream->lpVtbl->Release(stream);

    if (FAILED(hr) || !Root->Count)
    {
        XmlFree(&context.Menu); // partly read <menu>
        XmlFree(Root);
        XmlFallback(Root);
        return FALSE;
    }

    return TRUE;
}

VOID XmlFree(_Inout_ PTB_ITEM Root)
{
    for (ULONG i = 0; i < Root->Count; i++)
        XmlFree(&Root->Items[i]);

    if (Root->Items)
        HeapFree(GetProcessHeap(), 0, Root->Items);

    Root->Items = NULL;
    Root->Count = 0;
}

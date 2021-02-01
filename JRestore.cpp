#include "framework.h"
#include "JRestore.h"
#include <windowsx.h>
#include <uxtheme.h>
#include <commctrl.h>
#include <string>
#include <map>

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

#define MAX_LOADSTRING 100
#define ID_BTN_SAVE 110
#define ID_BTN_RESTORE 111
#define ID_BTN_CLEAR 112
#define ID_TABLE 113

struct WindowData
{
    LPTSTR wTitle;
    WINDOWPLACEMENT wPlacement;
    BOOL wVisible;
};

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

HWND hSave;
HWND hRestore;
HWND hClear;
HWND hTable;

int currentDpi;
std::map<HWND, WindowData> data;
BOOL widthChanged[12] = {};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int ScaleDpi(int originalSize);
void Resize(int newX, int newY);
void ResizeTable(int newX);
void Save(HWND hWnd);
void Restore(HWND hWnd);
void Clear();
BOOL CALLBACK EnumWindowsProc(_In_ HWND hwnd, _In_ LPARAM lParam);
void UpdateTable();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HANDLE hMutex = CreateMutex(NULL, TRUE, L"JRestore");

    if(hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        HWND hWnd = FindWindow(NULL, L"JRestore");

        if(hWnd != NULL)
        {
            FlashWindow(hWnd, TRUE);
        }

        return FALSE;
    }

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_JRESTORE, szWindowClass, MAX_LOADSTRING);

    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_JRESTORE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_JRESTORE));

    RegisterClassExW(&wcex);

    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, 100, 100, 1093, 614, nullptr, nullptr, hInstance, nullptr);

    if(!hWnd)
    {
        return FALSE;
    }

    int w = ScaleDpi(1093);
    int h = ScaleDpi(614);

    SetWindowPos(hWnd, (HWND)0, (GetSystemMetrics(SM_CXSCREEN) - w) / 2, (GetSystemMetrics(SM_CYSCREEN) - h) / 2, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    Save(hWnd);
    
    ShowWindow(hWnd, SW_MINIMIZE);

    MSG msg;

    while(GetMessage(&msg, nullptr, 0, 0))
    {
        if(IsDialogMessage(hWnd, &msg) == 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_CREATE:
    {
        currentDpi = GetDpiForWindow(hWnd);
        HFONT hFont = CreateFont(ScaleDpi(16), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));

        hSave = CreateWindow(WC_BUTTON, L"Save", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, ScaleDpi(421), 8, ScaleDpi(80), ScaleDpi(24), hWnd, (HMENU)ID_BTN_SAVE, hInst, nullptr);
        hRestore = CreateWindow(WC_BUTTON, L"Restore", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, ScaleDpi(506), 8, ScaleDpi(80), ScaleDpi(24), hWnd, (HMENU)ID_BTN_RESTORE, hInst, nullptr);
        hClear = CreateWindow(WC_BUTTON, L"Clear", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, ScaleDpi(592), 8, ScaleDpi(80), ScaleDpi(24), hWnd, (HMENU)ID_BTN_CLEAR, hInst, nullptr);

        INITCOMMONCONTROLSEX icex;
        icex.dwICC = ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icex);

        int top = ScaleDpi(24) + 16;
        RECT rect;
        GetClientRect(hWnd, &rect);

        hTable = CreateWindow(WC_LISTVIEW, L"", WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT, 0, top, rect.right, rect.bottom - top, hWnd, (HMENU)ID_TABLE, hInst, nullptr);
        ListView_SetExtendedListViewStyle(hTable, LVS_EX_COLUMNSNAPPOINTS | LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

        SendMessage(hSave, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hRestore, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hClear, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hTable, WM_SETFONT, (WPARAM)hFont, TRUE);

        SetWindowTheme(hTable, L"Explorer", NULL);

        LVCOLUMN lvColumn;
        lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_MINWIDTH;
        lvColumn.pszText = (LPTSTR)L"Title";
        lvColumn.cx = ScaleDpi(280);
        lvColumn.cxMin = 50;
        lvColumn.fmt = LVCFMT_LEFT;

        ListView_InsertColumn(hTable, 0, &lvColumn);

        lvColumn.cx = ScaleDpi(70);
        lvColumn.pszText = (LPTSTR)L"Flags";
        ListView_InsertColumn(hTable, 1, &lvColumn);
        lvColumn.pszText = (LPTSTR)L"ShowCmd";
        ListView_InsertColumn(hTable, 2, &lvColumn);
        lvColumn.pszText = (LPTSTR)L"Visible";
        ListView_InsertColumn(hTable, 3, &lvColumn);
        lvColumn.pszText = (LPTSTR)L"Min X";
        ListView_InsertColumn(hTable, 4, &lvColumn);
        lvColumn.pszText = (LPTSTR)L"Min Y";
        ListView_InsertColumn(hTable, 5, &lvColumn);
        lvColumn.pszText = (LPTSTR)L"Max X";
        ListView_InsertColumn(hTable, 6, &lvColumn);
        lvColumn.pszText = (LPTSTR)L"Max Y";
        ListView_InsertColumn(hTable, 7, &lvColumn);
        lvColumn.pszText = (LPTSTR)L"Left";
        ListView_InsertColumn(hTable, 8, &lvColumn);
        lvColumn.pszText = (LPTSTR)L"Top";
        ListView_InsertColumn(hTable, 9, &lvColumn);
        lvColumn.pszText = (LPTSTR)L"Right";
        ListView_InsertColumn(hTable, 10, &lvColumn);
        lvColumn.pszText = (LPTSTR)L"Bottom";
        ListView_InsertColumn(hTable, 11, &lvColumn);

        Button_Enable(hSave, FALSE);
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);

        switch(wmId)
        {
        case ID_BTN_SAVE:
            Save(hWnd);
            break;
        case ID_BTN_RESTORE:
            Restore(hWnd);
            break;
        case ID_BTN_CLEAR:
            Clear();
            break;
        }
        break;
    }
    case WM_SIZE:
    {
        int wmId = LOWORD(wParam);
        if(wmId == 0 || wmId == 2)
        {
            Resize(LOWORD(lParam), HIWORD(lParam));
        }
        break;
    }
    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        int minSize = ScaleDpi(300);
        lpMMI->ptMinTrackSize.x = minSize;
        lpMMI->ptMinTrackSize.y = minSize;
        break;
    }
    case WM_DPICHANGED:
    {
        currentDpi = HIWORD(wParam);

        RECT* const prcNewWindow = (RECT*)lParam;
        SetWindowPos(hWnd, (HWND)0, prcNewWindow->left, prcNewWindow->top, prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top, SWP_NOZORDER | SWP_NOACTIVATE);

        HFONT hFont = CreateFont(ScaleDpi(16), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));

        SendMessage(hSave, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hRestore, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hClear, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hTable, WM_SETFONT, (WPARAM)hFont, TRUE);

        Resize(prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top);

        break;
    }
    case WM_NOTIFY:
    {
        LPNMHEADER pNMHeader = (LPNMHEADER)lParam;
        if(pNMHeader->hdr.code == HDN_ENDTRACK)
        {
            widthChanged[pNMHeader->iItem] = TRUE;
        }
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int ScaleDpi(int original)
{
    return MulDiv(original, currentDpi, 96);
}

void Resize(int newX, int newY)
{
    int middle = (newX - ScaleDpi(80)) / 2;
    int top = ScaleDpi(24) + 16;

    SetWindowPos(hSave, (HWND)0, middle - ScaleDpi(85), 8, ScaleDpi(80), ScaleDpi(24), SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(hRestore, (HWND)0, middle, 8, ScaleDpi(80), ScaleDpi(24), SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(hClear, (HWND)0, middle + ScaleDpi(85), 8, ScaleDpi(80), ScaleDpi(24), SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(hTable, (HWND)0, 0, top, newX, newY - top, SWP_NOZORDER | SWP_NOACTIVATE);

    ResizeTable(newX);
}

void ResizeTable(int newX)
{
    SendMessage(hTable, WM_SETREDRAW, FALSE, 0);
    SCROLLBARINFO sbInfo;
    sbInfo.cbSize = sizeof(SCROLLBARINFO);

    GetScrollBarInfo(hTable, OBJID_VSCROLL, &sbInfo);

    int scrollBarWidth = GetSystemMetricsForDpi(SM_CYVSCROLL, currentDpi);

    if(sbInfo.dxyLineButton == 0)
    {
        scrollBarWidth = 0;
    }

    int minWidth = ScaleDpi(280) + ScaleDpi(70) * 11 + scrollBarWidth;

    if(newX > minWidth)
    {
        int newWidth = ScaleDpi(280) + newX - minWidth;
        int maxWidth = ScaleDpi(640);
        
        if(newWidth > maxWidth)
        {
            int newColWidth = (newX - maxWidth) / 11;

            if(!widthChanged[0])
            {
                ListView_SetColumnWidth(hTable, 0, newX - scrollBarWidth - (newColWidth * 11));
            }
            
            for(int i = 1; i < 12; i++)
            {
                if(!widthChanged[i])
                {
                    ListView_SetColumnWidth(hTable, i, newColWidth);
                }
            }
        }
        else
        {
            if(!widthChanged[0])
            {
                ListView_SetColumnWidth(hTable, 0, newWidth);
            }
            int scale70 = ScaleDpi(70);
            for(int i = 1; i < 12; i++)
            {
                if(!widthChanged[i])
                {
                    ListView_SetColumnWidth(hTable, i, scale70);
                }
            }
        }
    }
    else
    {
        if(!widthChanged[0])
        {
            ListView_SetColumnWidth(hTable, 0, ScaleDpi(280));
        }
        int scale70 = ScaleDpi(70);
        for(int i = 1; i < 12; i++)
        {
            if(!widthChanged[i])
            {
                ListView_SetColumnWidth(hTable, i, scale70);
            }
        }
    }
    SendMessage(hTable, WM_SETREDRAW, TRUE, 0);
}

void Save(HWND hWnd)
{
    Button_Enable(hSave, FALSE);
    Button_Enable(hRestore, TRUE);
    Button_Enable(hClear, TRUE);

    for(auto const& x : data)
    {
        delete[] x.second.wTitle;
    }
    data.clear();
    ListView_DeleteAllItems(hTable);

    EnumWindows(EnumWindowsProc, (LPARAM)nullptr);
    UpdateTable();

    std::wstring s = L"Saved ";
    s += std::to_wstring(data.size());
    s += L" window handles to the list!";
    
    MessageBox(hWnd, s.c_str(), L"Success", 0);
}

void Restore(HWND hWnd)
{
    Button_Enable(hSave, TRUE);
    int i = 0;

    for(auto const& x : data)
    {
        WINDOWPLACEMENT wPlacement = x.second.wPlacement;
        
        if(wPlacement.flags != WPF_RESTORETOMAXIMIZED)
        {
            wPlacement.flags = WPF_SETMINPOSITION;
        }
        else
        {
            wPlacement.flags = WPF_SETMINPOSITION | WPF_RESTORETOMAXIMIZED;
        }
        
        if(!x.second.wVisible)
        {
            if(wPlacement.showCmd == SW_MAXIMIZE)
            {
                HMONITOR hMonitor = MonitorFromRect(&wPlacement.rcNormalPosition, MONITOR_DEFAULTTOPRIMARY);

                MONITORINFO mInfo;
                mInfo.cbSize = sizeof(MONITORINFO);

                GetMonitorInfo(hMonitor, &mInfo);
                
                SetWindowPos(x.first, nullptr, mInfo.rcWork.left, mInfo.rcWork.top, mInfo.rcWork.right - mInfo.rcWork.left, mInfo.rcWork.bottom - mInfo.rcWork.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
            }
            wPlacement.showCmd = SW_HIDE;
        }
        else if(wPlacement.showCmd == SW_SHOWNORMAL)
        {
            wPlacement.showCmd = SW_SHOWNOACTIVATE;
        }
        else if(wPlacement.showCmd == SW_SHOWMINIMIZED)
        {
            wPlacement.showCmd = SW_SHOWMINNOACTIVE;
        }

        if(SetWindowPlacement(x.first, &wPlacement) != 0)
        {
            i++;
        }
    }

    std::wstring s = L"Restored ";
    s += std::to_wstring(i);
    s += L"(total ";
    s += std::to_wstring(data.size());
    s += L") windows!";

    MessageBox(hWnd, s.c_str(), L"Success", 0);
}

void Clear()
{
    Button_Enable(hSave, TRUE);
    Button_Enable(hRestore, FALSE);
    Button_Enable(hClear, FALSE);
    for(auto const& x : data)
    {
        delete[] x.second.wTitle;
    }
    data.clear();
    ListView_DeleteAllItems(hTable);

    RECT rect;
    GetWindowRect(hTable, &rect);
    ResizeTable(rect.right - rect.left);
}

BOOL CALLBACK EnumWindowsProc(_In_ HWND hWnd, _In_ LPARAM lParam)
{
    LPTSTR wTitle = new TCHAR[150];
    GetWindowText(hWnd, wTitle, 150);

    WINDOWPLACEMENT wPlacement;
    wPlacement.length = sizeof(WINDOWPLACEMENT);

    GetWindowPlacement(hWnd, &wPlacement);

    RECT rect = wPlacement.rcNormalPosition;
    if(!(rect.left == 0 && rect.top == 0 && rect.right == 0 && rect.bottom == 0))
    {
        WindowData wData;
        wData.wTitle = wTitle;
        wData.wPlacement = wPlacement;
        wData.wVisible = IsWindowVisible(hWnd);
        data.insert({ hWnd, wData });
    }
    else
    {
        delete[] wTitle;
    }
    return TRUE;
}

void UpdateTable()
{
    SendMessage(hTable, WM_SETREDRAW, FALSE, 0);
    int i = 0;
    for(auto const& x : data)
    {
        LVITEM lvItem = { 0 };
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = x.second.wTitle;
        ListView_InsertItem(hTable, &lvItem);

        std::wstring s = std::to_wstring(x.second.wPlacement.flags);
        lvItem.iSubItem = 1;
        lvItem.pszText = (LPTSTR)s.c_str();
        ListView_SetItem(hTable, &lvItem);

        s = std::to_wstring(x.second.wPlacement.showCmd);
        lvItem.iSubItem = 2;
        lvItem.pszText = (LPTSTR)s.c_str();
        ListView_SetItem(hTable, &lvItem);

        s = std::to_wstring(x.second.wVisible);
        lvItem.iSubItem = 3;
        lvItem.pszText = (LPTSTR)s.c_str();
        ListView_SetItem(hTable, &lvItem);

        s = std::to_wstring(x.second.wPlacement.ptMinPosition.x);
        lvItem.iSubItem = 4;
        lvItem.pszText = (LPTSTR)s.c_str();
        ListView_SetItem(hTable, &lvItem);

        s = std::to_wstring(x.second.wPlacement.ptMinPosition.y);
        lvItem.iSubItem = 5;
        lvItem.pszText = (LPTSTR)s.c_str();
        ListView_SetItem(hTable, &lvItem);

        s = std::to_wstring(x.second.wPlacement.ptMaxPosition.x);
        lvItem.iSubItem = 6;
        lvItem.pszText = (LPTSTR)s.c_str();
        ListView_SetItem(hTable, &lvItem);

        s = std::to_wstring(x.second.wPlacement.ptMaxPosition.y);
        lvItem.iSubItem = 7;
        lvItem.pszText = (LPTSTR)s.c_str();
        ListView_SetItem(hTable, &lvItem);

        s = std::to_wstring(x.second.wPlacement.rcNormalPosition.left);
        lvItem.iSubItem = 8;
        lvItem.pszText = (LPTSTR)s.c_str();
        ListView_SetItem(hTable, &lvItem);

        s = std::to_wstring(x.second.wPlacement.rcNormalPosition.top);
        lvItem.iSubItem = 9;
        lvItem.pszText = (LPTSTR)s.c_str();
        ListView_SetItem(hTable, &lvItem);

        s = std::to_wstring(x.second.wPlacement.rcNormalPosition.right);
        lvItem.iSubItem = 10;
        lvItem.pszText = (LPTSTR)s.c_str();
        ListView_SetItem(hTable, &lvItem);

        s = std::to_wstring(x.second.wPlacement.rcNormalPosition.bottom);
        lvItem.iSubItem = 11;
        lvItem.pszText = (LPTSTR)s.c_str();
        ListView_SetItem(hTable, &lvItem);

        i++;
    }

    SendMessage(hTable, WM_SETREDRAW, TRUE, 0);

    RECT rect;
    GetWindowRect(hTable, &rect);
    ResizeTable(rect.right - rect.left);
}

#include <windows.h>
#include <strsafe.h>
#include <tchar.h>
#include "resource.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

INT_PTR WINAPI DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define WM_TRAYICON                 (WM_USER + 10)
const UINT WM_TASKBARCREATED = ::RegisterWindowMessage(_T("TaskbarCreated"));

int APIENTRY WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    DialogBox(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG), NULL, DialogProc);

    return 0;
}

// 添加托盘图标
void ShellNotifyIconAdd(HWND hWnd)
{
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
    StringCchCopy(nid.szTip, _countof(nid.szTip), _T("Demo"));
    Shell_NotifyIcon(NIM_ADD, &nid);
}

// 删除托盘图标
void ShellNotifyIconDelete(HWND hWnd)
{
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize = sizeof(nid);
    nid.hWnd = hWnd;
    nid.uID = 0;
    nid.uFlags = 0;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

// 显示弹出菜单
void ShowPopupMenu(HWND hWnd, int xPos, int yPos, bool isTop)
{
    MENUITEMINFO mii = { 0 };
    TCHAR szBuf[MAX_PATH] = { 0 };

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_SUBMENU | MIIM_ID;
    mii.fType = MFT_STRING | MFT_OWNERDRAW;
    mii.dwTypeData = szBuf;

    HMENU hMainMenu = ::GetMenu(hWnd);
    if (hMainMenu)
    {
        HMENU hPopMenu = ::CreatePopupMenu();
        int nCount = ::GetMenuItemCount(hMainMenu);
        for (int i = 0; i < nCount; i++)
        {
            mii.cch = _countof(szBuf);
            ::GetMenuItemInfo(hMainMenu, i, TRUE, &mii);
            ::InsertMenuItem(hPopMenu, i, TRUE, &mii);
        }

        ::TrackPopupMenuEx(hPopMenu,
            TPM_LEFTALIGN | (isTop ? TPM_BOTTOMALIGN : TPM_TOPALIGN),
            xPos,
            yPos,
            hWnd,
            NULL
        );
    }
}

bool ShowForeground(HWND hWnd)
{
    HWND hForeWnd = ::GetForegroundWindow();
    DWORD dwForeID = ::GetWindowThreadProcessId(hForeWnd, NULL);
    DWORD dwCurID = ::GetCurrentThreadId();
    ::AttachThreadInput(dwCurID, dwForeID, TRUE);
    BOOL isSuc = ::SetForegroundWindow(hWnd);
    ::AttachThreadInput(dwCurID, dwForeID, FALSE);

    if (isSuc)
    {
        //return true;
    }

    if (!::IsWindowVisible(hWnd))
    {
        ::ShowWindow(hWnd, SW_SHOW);
    }

    if (IsIconic(hWnd))
    {
        ::ShowWindow(hWnd, SW_SHOWNORMAL);
    }

    return ::SetForegroundWindow(hWnd);
}

INT_PTR WINAPI DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // 资源管理器重启后重新建立托盘图标
    if (WM_TASKBARCREATED == uMsg)
    {
        ShellNotifyIconAdd(hWnd);
    }

    // 消息处理
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        ShellNotifyIconAdd(hWnd);
    }
    break;
    case WM_CLOSE:
    {
        ShellNotifyIconDelete(hWnd);
        ::EndDialog(hWnd, 0);
    }
    break;
    case WM_RBUTTONUP:
    {
        int xPos = ((int)(short)LOWORD(lParam));
        int yPos = ((int)(short)HIWORD(lParam));
        POINT pt = { xPos, yPos };
        ClientToScreen(hWnd, &pt);
        ShowPopupMenu(hWnd, pt.x, pt.y, false);
    }
    break;
    case WM_TRAYICON:
    {
        if (WM_LBUTTONDBLCLK == lParam)
        {
            ShowForeground(hWnd);
        }

        if (WM_RBUTTONUP == lParam)
        {
            // 防止托盘弹出菜单失去焦点后不消失
            static HWND hHideWnd = CreateWindowEx(0L,
                _T("Static"),
                _T(""),
                WS_CHILDWINDOW,
                0,
                0,
                0,
                0,
                HWND_MESSAGE,
                NULL,
                GetModuleHandle(NULL),
                NULL
            );

            if (hHideWnd && ::SetForegroundWindow(hHideWnd))
            {
                POINT pt = { 0 };
                ::GetCursorPos(&pt);
                ShowPopupMenu(hWnd, pt.x, pt.y, false);
            }
        }
    }
    break;
    case WM_COMMAND:
    {
        //消息源   HIWORD(wParam)      LOWORD(wParam)     lParam 
        //菜单     0                   菜单ID             0
        //快捷键   1                   快捷键ID           0
        //控件     控件定义的通知码    控件ID             控件窗口句柄
        UINT nNotify = HIWORD(wParam);
        UINT nID = LOWORD(wParam);
        HWND hWndFrom = (HWND)lParam;

        switch (nID)
        {
        case ID_FILE_EXIT:
        case IDOK:
        case IDCANCEL:
        {
            ::PostMessage(hWnd, WM_CLOSE, 0, 0);
        }
        }
    }
    break;
    default:
        return FALSE;
    }

    return TRUE;
}
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

bool next_click_replace = false;

LRESULT CALLBACK hooked_mouse(
    _In_ int    nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms644986(v=vs.85)#parameters
    if( nCode < HC_ACTION )
        return CallNextHookEx( nullptr, nCode, wParam, lParam );

    PMSLLHOOKSTRUCT lp = reinterpret_cast<PMSLLHOOKSTRUCT>(lParam);

    // when button is down and click needs to be replaced, send right click mouse event and return -1 (ignore current click)
    if( wParam == WM_LBUTTONDOWN && next_click_replace )
    {
        next_click_replace = false;

        // right click simulation
        mouse_event( MOUSEEVENTF_RIGHTDOWN, lp->pt.x, lp->pt.y, NULL, NULL );
        mouse_event( MOUSEEVENTF_RIGHTUP, lp->pt.x, lp->pt.y, NULL, NULL );

        // return -1 (ignore current click)
        return -1;
    }

    return CallNextHookEx( NULL, nCode, wParam, lParam );
}

extern LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    if( wParam == VK_ESCAPE )
        exit( 0 );

    switch( msg )
    {
        case WM_PARENTNOTIFY: // button click idk
        {
            if( wParam == WM_LBUTTONDOWN )
                next_click_replace = true;
            break;
        }

        case WM_SYSCOMMAND:
            if( (wParam & 0xfff0) == SC_KEYMENU ) // Disable ALT application menu
                return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage( 0 );
            return 0;
    }

    return ::DefWindowProc( hWnd, msg, wParam, lParam );
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow )
{
    // place low level hook
    auto mouse_hook = SetWindowsHookExA( WH_MOUSE_LL, hooked_mouse, hInstance, NULL );

    // create new class for the window
    WNDCLASSEX wc = { sizeof( WNDCLASSEX ), 0, WndProc, 0L, 0L, GetModuleHandleA( NULL ), NULL, NULL, NULL, NULL, L"drag", NULL };
    ::RegisterClassEx( &wc );

    // create window
    auto hwnd = ::CreateWindow( wc.lpszClassName, L"drag", WS_SYSMENU | WS_VISIBLE, 0, 0, 120, 100, NULL, NULL, wc.hInstance, NULL );

    // create button inside window as child
    HWND hwndButton = CreateWindowEx( 0, L"BUTTON",
                                      L"RIGHTCLICK",
                                      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_FLAT,
                                      0, // x
                                      0, // y
                                      120, // height
                                      60, // width
                                      hwnd,
                                      (HMENU) 5000,
                                      wc.hInstance,
                                      NULL );

    // force topmost
    SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

    // process messages
    MSG msg;
    while( GetMessage( &msg, NULL, 0, 0 ) > 0 )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    // cleanup
    UnhookWindowsHookEx( mouse_hook );
    return 0;
}
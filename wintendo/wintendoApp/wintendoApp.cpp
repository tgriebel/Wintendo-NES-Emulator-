// wintendoApp.cpp : Defines the entry point for the application.
//


#include <windows.h>
#include <combaseapi.h>
#include <shobjidl.h> 
#include <ole2.h>
#include <ObjBase.h>
#include <map>
#include <comdef.h>
#include "stdafx.h"
#include "wintendoApp.h"
#include "..\wintendoCore\wintendo_api.h"
#include "..\wintendoCore\input.h"


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HWND hWnd;

HBITMAP hBitmap;
HBITMAP nameTableImage;
HBITMAP paletteImage;
HBITMAP patternTable0Image;
HBITMAP patternTable1Image;

std::map<uint32_t, uint32_t> keyMap;
std::map<uint32_t, std::string> keyToAscii;

int32_t key = 0;

int32_t displayScalar = 2;
int32_t nesWidth = 256;
int32_t nesHeight = 240;
int32_t debugAreaX = 1024;
int32_t debugAreaY = 0;

unsigned frameBuffer[61440];
unsigned nameTable[245760];
unsigned paletteDebug[32];
unsigned paletteTable0Debug[16384];
unsigned paletteTable1Debug[16384];
unsigned frameSwap = 0;

static bool reset = true;
const char* nesFilePath = "Games/Contra.nes";

DWORD WINAPI EmulatorThread( LPVOID lpParameter )
{


	while( true )
	{
		if ( reset )
		{
			InitSystem( nesFilePath );
			reset = false;
		}

		RunFrame();
		CopyFrameBuffer( frameBuffer, 245760 );
		CopyNametable( nameTable, 983040 );
		CopyPalette( paletteDebug, 128 );
		CopyPatternTable0( paletteTable0Debug, 65535);
		CopyPatternTable1( paletteTable1Debug, 65535);

		hBitmap = CreateBitmap( nesWidth, nesHeight, 1, 32, frameBuffer );
		nameTableImage = CreateBitmap( 2 * nesWidth, 2 * nesHeight, 1, 32, nameTable );
		paletteImage = CreateBitmap( 16, 2, 1, 32, paletteDebug );
		patternTable0Image = CreateBitmap( 128, 128, 1, 32, paletteTable0Debug );
		patternTable1Image = CreateBitmap( 128, 128, 1, 32, paletteTable1Debug );

		RedrawWindow( hWnd, NULL, NULL, RDW_INVALIDATE );
	}

	return 0;
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	using namespace std;

	unsigned int sharedData = 0;

	DWORD threadID;
	HANDLE emulatorThreadHandle = CreateThread( 0, 0, EmulatorThread, &sharedData, 0, &threadID );

	if ( emulatorThreadHandle <= 0 )
		return 0;

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINTENDOAPP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINTENDOAPP));

    MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	CloseHandle(emulatorThreadHandle);

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINTENDOAPP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WINTENDOAPP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   keyMap['A'] = BUTTON_LEFT;
   keyMap['D'] = BUTTON_RIGHT;
   keyMap['W'] = BUTTON_UP;
   keyMap['S'] = BUTTON_DOWN;
   keyMap['G'] = BUTTON_SELECT;
   keyMap['H'] = BUTTON_START;
   keyMap['J'] = BUTTON_B;
   keyMap['K'] = BUTTON_A;

   keyToAscii[BUTTON_A] = "A";
   keyToAscii[BUTTON_B] = "B";
   keyToAscii[BUTTON_START] = "STR";
   keyToAscii[BUTTON_SELECT] = "SEL";
   keyToAscii[BUTTON_LEFT] = "L";
   keyToAscii[BUTTON_RIGHT] = "R";
   keyToAscii[BUTTON_UP] = "U";
   keyToAscii[BUTTON_DOWN] = "D";

   int dwStyle = ( WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX );

   RECT wr = { 0, 0, displayScalar * nesWidth + debugAreaX, displayScalar * nesHeight + debugAreaY };
   AdjustWindowRect( &wr, dwStyle, FALSE );

   hWnd = CreateWindowW( szWindowClass, szTitle, dwStyle,
      CW_USEDEFAULT, 0, ( wr.right - wr.left ), ( wr.bottom - wr.top ), nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


void OpenNesGame()
{
	// https://docs.microsoft.com/en-us/windows/win32/learnwin32/example--the-open-dialog-box
	HRESULT hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );

	if (SUCCEEDED(hr))
	{
		IFileOpenDialog* pFileOpen;

		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			// Show the Open dialog box.
			hr = pFileOpen->Show(NULL);

			// Get the file name from the dialog box.
			if (SUCCEEDED(hr))
			{
				IShellItem* pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr))
				{
					PWSTR filePath = nullptr;
					hr = pItem->GetDisplayName( SIGDN_FILESYSPATH, &filePath );

					// Display the file name to the user.
					if (SUCCEEDED(hr))
					{
						// MessageBox( NULL, filePath, L"File Path", MB_OK );
						CoTaskMemFree( filePath );
						_bstr_t b( filePath );
						nesFilePath = b;
						SetGameName( nesFilePath );
						reset = true;
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch ( message )
	{
	case WM_CREATE:
	{
		
	}
	break;

	case WM_COMMAND:
	{
		int wmId = LOWORD( wParam );
		// Parse the menu selections:
		switch ( wmId )
		{
		case IDM_ABOUT:
			DialogBox( hInst, MAKEINTRESOURCE( IDD_ABOUTBOX ), hWnd, About );
			break;
		case ID_FILE_OPEN:
			OpenNesGame();
			break;
		case IDM_EXIT:
			DestroyWindow( hWnd );
			break;
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
		}
	}
	break;

	case WM_PAINT:
	{
		PAINTSTRUCT     ps;
		HDC             hdc;
		BITMAP          screenBitmap;
		BITMAP          ntBitmap;
		BITMAP          palBitmap;
		BITMAP          pattern0Bitmap;
		BITMAP          pattern1Bitmap;
		HDC             hdcMem;
		HGDIOBJ         oldBitmap;

		hdc = BeginPaint( hWnd, &ps );
		
		hdcMem = CreateCompatibleDC( hdc );
		oldBitmap = SelectObject( hdcMem, hBitmap );

		GetObject( hBitmap, sizeof( screenBitmap ), &screenBitmap );
		StretchBlt( hdc, 0, 0, displayScalar * screenBitmap.bmWidth, displayScalar * screenBitmap.bmHeight, hdcMem, 0, 0, screenBitmap.bmWidth, screenBitmap.bmHeight, SRCCOPY );
		SelectObject( hdcMem, oldBitmap );

		oldBitmap = SelectObject( hdcMem, nameTableImage );

		GetObject( nameTableImage, sizeof( ntBitmap ), &ntBitmap );
		StretchBlt( hdc, displayScalar * screenBitmap.bmWidth + 10, 0, ntBitmap.bmWidth, ntBitmap.bmHeight, hdcMem, 0, 0, ntBitmap.bmWidth, ntBitmap.bmHeight, SRCCOPY );

		oldBitmap = SelectObject( hdcMem, paletteImage );

		GetObject( paletteImage, sizeof( palBitmap ), &palBitmap );
		StretchBlt( hdc, displayScalar * screenBitmap.bmWidth + 20 + ntBitmap.bmWidth, 0, 10 * palBitmap.bmWidth, 10 * palBitmap.bmHeight, hdcMem, 0, 0, palBitmap.bmWidth, palBitmap.bmHeight, SRCCOPY );
		
		oldBitmap = SelectObject( hdcMem, patternTable0Image );

		GetObject( patternTable0Image, sizeof(pattern0Bitmap), &pattern0Bitmap);
		StretchBlt(hdc, displayScalar * screenBitmap.bmWidth + 20 + ntBitmap.bmWidth, 50, 128, 128, hdcMem, 0, 0, pattern0Bitmap.bmWidth, pattern0Bitmap.bmHeight, SRCCOPY);

		oldBitmap = SelectObject( hdcMem, patternTable1Image );

		GetObject( patternTable1Image, sizeof(pattern1Bitmap), &pattern1Bitmap );
		StretchBlt( hdc, displayScalar * screenBitmap.bmWidth + 20 + ntBitmap.bmWidth + 140, 50, 256, 256, hdcMem, 0, 0, pattern1Bitmap.bmWidth, pattern1Bitmap.bmHeight, SRCCOPY);

		SelectObject( hdcMem, oldBitmap );
		DeleteDC( hdcMem );
		EndPaint( hWnd, &ps );

		DeleteObject( hBitmap );
		DeleteObject( nameTableImage );
		DeleteObject( paletteImage );
		DeleteObject( patternTable0Image );
		DeleteObject( patternTable1Image );

		::ReleaseDC( NULL, hdc );
	}
	break;

	case WM_KEYDOWN:
	{
		const uint32_t capKey = toupper( (int)wParam );
		const uint32_t key = keyMap[capKey];

		StoreKey( key );
	}
	break;

	case WM_KEYUP:
	{
		const uint32_t capKey = toupper( (int)wParam );
		const uint32_t key = keyMap[capKey];

		ReleaseKey( key );
	}
	break;

	case WM_DESTROY:
	{
		DeleteObject( hBitmap );
		DeleteObject( nameTableImage );
		DeleteObject( paletteImage );
		DeleteObject( patternTable0Image );
		DeleteObject( patternTable1Image );
		PostQuitMessage( 0 );
	}
	break;

	default:
		return DefWindowProc( hWnd, message, wParam, lParam );
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

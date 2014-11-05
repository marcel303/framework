#include <string>
#include <windows.h>
#include "DisplayWindows.h"

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

DisplayWindows::DisplayWindows(int x, int y, int width, int height, bool fullscreen) : Display(x, y, width, height, fullscreen)
{
	m_hWnd = 0;

	static bool isWindowClassRegistered = false;

	if (!isWindowClassRegistered)
	{
		RegisterWindowClass();

		isWindowClassRegistered = true;
	}

	// TODO: Fullscreen true/false.
	WinCreateWindow(x, y, w, h);
}

DisplayWindows::~DisplayWindows()
{
	DestroyWindow(m_hWnd);
}

int DisplayWindows::GetWidth()
{
	RECT rect;
	
	GetWindowRect(m_hWnd, &rect);
	
	return rect.right - rect.left;
}

int DisplayWindows::GetHeight()
{
	RECT rect;
	
	GetWindowRect(m_hWnd, &rect);
	
	return rect.bottom - rect.top;
}

void* DisplayWindows::Get(const std::string& name)
{
	if (name == "hWnd")
	{
		return m_hWnd;
	}	
	
	return 0;
}

bool DisplayWindows::Update()
{
	bool bQuit = false;
	
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			bQuit = true;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	if (bQuit)
	{
		return false;
	}

    return true;
}

bool DisplayWindows::RegisterWindowClass()
{
    WNDCLASS wc;

    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = GetModuleHandle(0);
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = TEXT("DisplayWindows");

    RegisterClass(&wc);

	return true;
}

bool DisplayWindows::WinCreateWindow(int x, int y, int w, int h)
{
    m_hWnd = CreateWindow(
		TEXT("DisplayWindows"), // ClassName.
		TEXT("Window"),         // Caption.
		(1 ? WS_VISIBLE | WS_POPUP : WS_VISIBLE | WS_MINIMIZEBOX | WS_SYSMENU | WS_SIZEBOX), // Style.
		x,    // Left.
		y,    // Top.
		w,    // Width.
		h,    // Height.
		NULL, // Parent.
		NULL, // Menu.
		GetModuleHandle(0), NULL);

	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;

	return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
		case WM_CREATE:
		{
			return 0;
		}
		break;

		case WM_CLOSE:
		{
			PostQuitMessage(0);
			
			return 0;
		}
		break;

		case WM_DESTROY:
		{
			return 0;
		}
		break;

		case WM_KEYDOWN:
		{
			switch (wParam)
			{
				case VK_ESCAPE:
				{
					PostQuitMessage(0);
					
					return 0;
				}
				break;
			}

			return 0;
		}
		break;

		default:
		{
	        return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	}

	return 0;
}

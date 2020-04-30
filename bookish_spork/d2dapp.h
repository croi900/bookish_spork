#pragma once

#include <windows.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>



//chestii de copiat pt ca asa zice sefu

template<class Interface>
inline void SafeRelease(
	Interface** ppInterfaceToRelease
)
{
	if (*ppInterfaceToRelease != NULL)
	{
		(*ppInterfaceToRelease)->Release();

		(*ppInterfaceToRelease) = NULL;
	}
}


#ifndef Assert
#if defined( DEBUG ) || defined( _DEBUG )
#define Assert(b) do {if (!(b)) {OutputDebugStringA("Assert: " #b "\n");}} while(0)
#else
#define Assert(b)
#endif //DEBUG || _DEBUG
#endif

#pragma comment(lib, "d2d1")

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

class DirectApp
{
public:
	DirectApp();
	~DirectApp();
	HRESULT Init();
	void Start();
	void RunMsgLoop();
private:
	HRESULT CreateDIR();//Create resources independent of the device
	HRESULT CreateDR();//Create resources dependent of the device
	void DiscardResources(); //Discard resources dependent of the device

	HRESULT OnRender();

	void OnResize(
		UINT width,
		UINT height
	);


	// Window
	static LRESULT CALLBACK WndProc(
		HWND hWnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam
	);

	HWND hWnd;
	ID2D1Factory* pDirect2dFactory;
	ID2D1HwndRenderTarget* pRenderTarget;
	ID2D1SolidColorBrush* pRedBrush;
	ID2D1SolidColorBrush* pBlueBrush;

};

DirectApp::DirectApp() : hWnd(NULL), pDirect2dFactory(NULL), pRenderTarget(NULL), pRedBrush(NULL), pBlueBrush(NULL)
{
}

DirectApp::~DirectApp()
{
	SafeRelease(&pDirect2dFactory);
	SafeRelease(&pRenderTarget);
	SafeRelease(&pRedBrush);
	SafeRelease(&pBlueBrush);
}

void DirectApp::RunMsgLoop()
{
	MSG msg;
	while (GetMessage(&msg, NULL, NULL, NULL))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

HRESULT DirectApp::Init()
{
	HRESULT hr;
	hr = CreateDIR();

	if (SUCCEEDED(hr))
	{

		//create window and register it
		WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = DirectApp::WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(LONG_PTR);
		wcex.hInstance = HINST_THISCOMPONENT;
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
		wcex.lpszClassName = L"D2DDemoApp";

		RegisterClassEx(&wcex);


		//scale window according to dpi
		FLOAT dpiX, dpiY;
#pragma warning(suppress:4996)
		pDirect2dFactory->GetDesktopDpi(&dpiX, &dpiY);
		
		hWnd = CreateWindowA(
			"D2DDemoApp",
			"Direct2D Demo App",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			static_cast<UINT>(ceil(640.f * dpiX / 96.f)),
			static_cast<UINT>(ceil(480.f * dpiY / 96.f)),
			NULL,
			NULL,
			HINST_THISCOMPONENT,
			this
		);
		hr = hWnd ? S_OK : E_FAIL;
		if (SUCCEEDED(hr))
		{
			ShowWindow(hWnd, SW_SHOWNORMAL);
			UpdateWindow(hWnd);
		}
	}
	return hr;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		{
			DirectApp app;
			if (SUCCEEDED(app.Init()))
			{
				app.RunMsgLoop();
			}
		}
		CoUninitialize();
	}
	return 0;
}

HRESULT DirectApp::CreateDIR()
{
	HRESULT hr = S_OK;

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pDirect2dFactory);

	return hr;
}

HRESULT DirectApp::CreateDR()
{
	HRESULT hr = S_OK;
	if (!pRenderTarget)
	{
		RECT rc;
		GetClientRect(hWnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(
			rc.right - rc.left, rc.bottom - rc.top
		);

		hr = pDirect2dFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hWnd, size),
			&pRenderTarget
		);

		if (SUCCEEDED(hr))
		{
			hr = pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Red), &pRedBrush
			);
		}
		if (SUCCEEDED(hr))
		{
			hr = pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Blue), &pBlueBrush
			);
		}
	}
	return hr;
}

void DirectApp::DiscardResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&pRedBrush);
	SafeRelease(&pBlueBrush);
}

LRESULT CALLBACK DirectApp::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	if (message == WM_CREATE)
	{
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
		DirectApp* pApp = static_cast<DirectApp*>(pcs->lpCreateParams);

		::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pApp));
		result = 1;
	}
	else
	{
		DirectApp* pApp = reinterpret_cast<DirectApp*>(static_cast<LONG_PTR>(::GetWindowLongW(hwnd, GWLP_USERDATA)));
		bool wasHandled = false;

		if (pApp)
		{
			switch (message)
			{
			case WM_SIZE:
			{
				UINT width = LOWORD(lParam);
				UINT height = HIWORD(lParam);
				pApp->OnResize(width, height);
			}
			result = 0;
			wasHandled = true;
			break;

			case WM_DISPLAYCHANGE:
			{
				InvalidateRect(hwnd, NULL, FALSE);
			}
			result = 0;
			wasHandled = true;
			break;

			case WM_PAINT:
			{
				pApp->OnRender();
				ValidateRect(hwnd, NULL);
			}
			result = 0;
			wasHandled = true;
			break;
			case WM_DESTROY:
			{
				PostQuitMessage(0);
			}
			result = 1;
			wasHandled = true;
			break;
			}
		}
		if (!wasHandled)
		{
			result = DefWindowProc(hwnd, message, wParam, lParam);
		}
	}
	return result;
}

HRESULT DirectApp::OnRender()
{
	HRESULT hr = S_OK;
	hr = CreateDR();

	if (SUCCEEDED(hr))
	{
		pRenderTarget->BeginDraw();
		pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
		D2D1_SIZE_F rtSize = pRenderTarget->GetSize();

		int width = static_cast<int>(rtSize.width);
		int height = static_cast<int>(rtSize.height);
		for (int x = 0; x < width; x += 10)
		{
			pRenderTarget->DrawLine(
				D2D1::Point2F(static_cast<FLOAT>(x), 0.0f),
				D2D1::Point2F(static_cast<FLOAT>(x), rtSize.height),
				pRedBrush,
				0.5f
			);
		}

		for (int y = 0; y < height; y += 10)
		{
			pRenderTarget->DrawLine(
				D2D1::Point2F(0.0f, static_cast<FLOAT>(y)),
				D2D1::Point2F(rtSize.width, static_cast<FLOAT>(y)),
				pBlueBrush,
				0.5f
			);
		}

		D2D1_RECT_F rect1 = D2D1::RectF(rand() % width, rand() % height, rand() % width, rand() % height);
		D2D1_RECT_F rect2 = D2D1::RectF(rand() % width, rand() % height, rand() % width, rand() % height);

		pRenderTarget->FillRectangle(&rect1, pRedBrush);
		pRenderTarget->FillRectangle(&rect2, pBlueBrush);

		hr = pRenderTarget->EndDraw();
	}

	if (hr == D2DERR_RECREATE_TARGET)
	{
		hr = S_OK;
		DiscardResources();
	}
	return hr;
}

void DirectApp::OnResize(UINT width, UINT height)
{
	if (pRenderTarget)
	{
		pRenderTarget->Resize(D2D1::SizeU(width, height));
	}
}

void DirectApp::Start()
{
	Init();
	RunMsgLoop();
}
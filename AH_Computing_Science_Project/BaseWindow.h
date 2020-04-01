#pragma once

#include "CA_pch.h"
#include "resource.h"
#include "CustomMessages.h"

/* Base window class based on example from
https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
*/

// This class allows me to create and interact with windows as individual objects which simplifies the code required.

template <class DERIVED_TYPE>
class BaseWindow
{
public:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		DERIVED_TYPE* pThis = NULL;

		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
			pThis = (DERIVED_TYPE*)pCreate->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

			pThis->m_hwnd = hwnd;			
		}
		else
		{
			pThis = (DERIVED_TYPE*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		}
		if (pThis)
		{
			return pThis->HandleMessage(uMsg, wParam, lParam);
		}
		else
		{
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}

	BaseWindow() : m_hwnd(NULL), m_inst(NULL) { }

	BOOL Create(
		PCWSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = 0,
		HMENU hMenu = 0
	)
	{
		WNDCLASS wc = { 0 };

		m_inst = GetModuleHandle(NULL);

		wc.lpfnWndProc = DERIVED_TYPE::WindowProc;
		wc.hInstance = m_inst;
		wc.lpszClassName = ClassName();

		RegisterClass(&wc);

		m_hwnd = CreateWindowEx(
			dwExStyle, ClassName(), lpWindowName, dwStyle, x, y,
			nWidth, nHeight, hWndParent, hMenu, m_inst, this
		);

		return (m_hwnd ? TRUE : FALSE);
	}

	HWND Window() const { return m_hwnd; }

protected:

	virtual PCWSTR  ClassName() const = 0;
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	float toPixels(float x) { 
		return x * DPIScale;
	} // Converts device independent pixels to physical pixels.

	float fromPixels(float x) {
		return x / DPIScale;
	} // reverse of toPixels

	//D2D1_RECT_F rectToPix(D2D1_RECT_F rect) {
	//	D2D1_RECT_F r;
	//	r.left = toPixels(rect.left);
	//	r.top = toPixels(rect.top);
	//	r.right = toPixels(rect.right);
	//	r.bottom = toPixels(rect.bottom);
	//	return r;
	//}

	D2D1_RECT_F rectFromPix(D2D1_RECT_F rect) {
		D2D1_RECT_F r;
		r.left = fromPixels(rect.left);
		r.top = fromPixels(rect.top);
		r.right = fromPixels(rect.right);
		r.bottom = fromPixels(rect.bottom);
		return r;
	}

	HWND m_hwnd;
	HINSTANCE m_inst;

	float DPIScale = 1.0f;
};


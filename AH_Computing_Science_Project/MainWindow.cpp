#include "MainWindow.h"

PCWSTR MainWindow::ClassName() const
{
	WCHAR windowName[MAX_LOADSTRING];
	LoadStringW(m_inst, IDS_MAINWINDOWNAME, windowName, MAX_LOADSTRING);
	return windowName;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		break;
	case WM_PAINT:
		break;
	}

	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

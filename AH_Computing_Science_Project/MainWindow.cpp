#include "MainWindow.h"
#include "Graphics.h"
#include "Network.h"

PCWSTR MainWindow::ClassName() const
{
	return (PCWSTR) m_className;
}

MainWindow::MainWindow(Graphics * graphics, Network * nw) : pRenderTarget(NULL)
{
	LoadStringW(m_inst, IDS_MAINWINDOWNAME, m_className, MAX_LOADSTRING);

	myGraphics = graphics;
	network = nw;
	m_rect = RECT();

	myGraphics->getWriteFactory()->CreateTextFormat(
		L"Ariel",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		48.0f,
		L"en-uk",
		&pTitleTextFormat
	);
	pTitleTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		m_findGameButton = CreateWindowEx(NULL, L"BUTTON", L"Find Game", WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 32, 256, 224, 32, Window(), NULL, m_inst, NULL);
		m_settingsButton = CreateWindowEx(NULL, L"BUTTON", L"Settings", WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 32, 320, 224, 32, Window(), NULL, m_inst, NULL);
		m_exitButton = CreateWindowEx(NULL, L"BUTTON", L"Quit to Desktop", WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 32, 384, 224, 32, Window(), NULL, m_inst, NULL);
		break;
	case WM_SIZE:
		GetClientRect(m_hwnd, &m_rect);
		discardGraphicResources();
		InvalidateRect(m_hwnd, NULL, TRUE);

		if (m_logInHandle != NULL) {
			MoveWindow(m_logInHandle, m_rect.right / 2 - 256, 32 , 512, m_rect.bottom - 64, true);
		}

		break;
	case WM_PAINT:
		onPaint();
		//InvalidateRect(m_logInHandle, NULL, false);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = 624;
		lpMMI->ptMinTrackSize.y = 736;
		break;
	}
	case CA_SHOWMAIN:
		ShowWindow(m_findGameButton, wParam);
		ShowWindow(m_settingsButton, wParam);
		ShowWindow(m_exitButton, wParam);

		if (wParam == SW_SHOW) {
			windowShown = true;
		}
		else {
			windowShown = false;
		}
		break;

	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
			if ((HWND)lParam == m_exitButton) {
				PostQuitMessage(0);
				break;
			}
			else if ((HWND)lParam == m_findGameButton) {
				network->joinMatchmakingQueue();
			}
			break;
		}
		break;
	}

	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void MainWindow::onPaint() {
	createGraphicResources();

	// Begin Draw
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hwnd, &ps);

	pRenderTarget->BeginDraw();

	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF(0.97f,0.97f,0.98f)));

	// Draw Window
	pRenderTarget->DrawTextW(
		L"Reflex",
		(UINT32)6,
		pTitleTextFormat,
		D2D1::RectF(32.0f, 32.0f, m_rect.right - 32.0f, 32.0f),
		bBlack
	);


	//End Draw
	pRenderTarget->EndDraw();

	EndPaint(m_hwnd, &ps);
}

void MainWindow::createGraphicResources()
{
	HRESULT hr = S_OK;
	if (pRenderTarget == NULL)
	{
		D2D1_SIZE_U size = D2D1::SizeU(m_rect.right, m_rect.bottom);

		hr = myGraphics->getFactory()->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, size),
			&pRenderTarget);

		pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &bBlack);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBombBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.2f), &pExplosionBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green), &pFoodBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &pBorderBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.5f, 1.0f, 0.5f, 0.15f), &pSightRangeBrush);
	}
}

void MainWindow::discardGraphicResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&bBlack);
}

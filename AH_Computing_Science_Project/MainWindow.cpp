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

	myGraphics->getWriteFactory()->CreateTextFormat(
		L"Ariel",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		24.0f,
		L"en-uk",
		&pMenuTextFormat
	);
	pMenuTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
	{
		UINT dpi = GetDpiForWindow(m_hwnd);
		DPIScale = dpi / 96.0f;

		GetClientRect(m_hwnd, &m_rect);

		// Main Menu Controls
		m_findGameButton = CreateWindowEx(NULL, L"BUTTON", L"Find Game", WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, toPixels(32), toPixels(256), toPixels(224), toPixels(32), Window(), NULL, m_inst, NULL);
		m_settingsButton = CreateWindowEx(NULL, L"BUTTON", L"Settings", WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, toPixels(32), toPixels(320), toPixels(224), toPixels(32), Window(), NULL, m_inst, NULL);
		m_exitButton = CreateWindowEx(NULL, L"BUTTON", L"Quit to Desktop", WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, toPixels(32), toPixels(384), toPixels(224), toPixels(32), Window(), NULL, m_inst, NULL);

		// Finding Game Controls
		m_cancelButton = CreateWindowEx(NULL, L"BUTTON", L"Cancel", WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, m_rect.right / 2 - toPixels(150), m_rect.bottom - toPixels(32 + 48), toPixels(300), toPixels(48), Window(), NULL, m_inst, NULL);

		break;
	}
	case WM_DPICHANGED:
	{
		UINT dpi = GetDpiForWindow(m_hwnd);
		DPIScale = dpi / 96.0f;
		break;
	}
	case WM_SIZE:
		GetClientRect(m_hwnd, &m_rect);
		discardGraphicResources();
		InvalidateRect(m_hwnd, NULL, TRUE);

		if (m_logInHandle != NULL) {
			MoveWindow(m_logInHandle, m_rect.right / 2 - toPixels(256), toPixels(32) , toPixels(512), m_rect.bottom - toPixels(64), true);
		}

		if (m_gameWindowHandle != NULL) {
			MoveWindow(m_gameWindowHandle, 0, 0, m_rect.right, m_rect.bottom, true);
		}

		MoveWindow(m_cancelButton, m_rect.right / 2 - toPixels(150), m_rect.bottom - toPixels(32 + 48), toPixels(300), toPixels(48), true);

		break;
	case WM_PAINT:
		onPaint();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = toPixels(624);
		lpMMI->ptMinTrackSize.y = toPixels(736);
		break;
	}
	case CA_SHOWMAIN:
		ShowWindow(m_findGameButton, wParam);
		ShowWindow(m_settingsButton, wParam);
		ShowWindow(m_exitButton, wParam);

		if (wParam == SW_SHOW) {
			windowShown = true;
			if (lParam != NULL) {
				m_userId = *(int32_t*)lParam;
			}
		}
		else {
			windowShown = false;
		}
		break;

	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
			if (findingGame) {
				if ((HWND)lParam == m_cancelButton) {
					network->leaveMatchmakingQueue();
					SendMessage(m_hwnd, CA_SHOWMAIN, SW_SHOW, NULL);
					ShowWindow(m_cancelButton, SW_HIDE);
					findingGame = false;
					
				}
			}
			else{
				if ((HWND)lParam == m_exitButton) {
					PostQuitMessage(0);
				}
				else if ((HWND)lParam == m_findGameButton) {
					findingGame = network->joinMatchmakingQueue();
					if (findingGame) {
						SendMessage(m_hwnd, CA_SHOWMAIN, SW_HIDE, NULL);
						ShowWindow(m_cancelButton, SW_SHOW);						
						findGameTimer = std::chrono::steady_clock::now();

						InvalidateRect(m_cancelButton, NULL, false);

						while (findingGame)
						{
							MSG msg;
							if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
							{
								if (msg.message == WM_QUIT ||
									msg.message == WM_DESTROY ||
									msg.message == WM_CLOSE)
								{
									findingGame = false;
									PostQuitMessage(0);									
									break;
								}
								TranslateMessage(&msg);
								DispatchMessage(&msg);
							}
							else {
								InvalidateRect(m_hwnd, NULL, false);
								if (network->checkForGame(m_userId)) {
									findingGame = false;
									ShowWindow(m_gameWindowHandle, SW_SHOW);
									SendMessage(m_gameWindowHandle, CA_SHOWGAME,NULL,NULL);
								}
								Sleep(1);
							}
						}
					}
				}				
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
	
	if (findingGame) {
		pRenderTarget->DrawTextW(
			L"searching for game...",
			_countof(L"searching for game..."),
			pMenuTextFormat,
			D2D1::RectF(toPixels(32.0f), m_rect.bottom / 2 - toPixels(16.0f), m_rect.right - toPixels(32.0f), m_rect.bottom / 2 - toPixels(16.0f)),
			bBlack
		);

		auto currentTime = std::chrono::steady_clock::now();
		int elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(currentTime - findGameTimer).count();
		int elapsedMinutes = elapsedSeconds / 60; // Integer division so will round down
		elapsedSeconds -= elapsedMinutes * 60; // Not equivelent to elapsedSeconds = 0 since elapsed minutes is rounded down.
		wchar_t buff[16];
		int length = swprintf_s(buff, L"%02d:%02d", elapsedMinutes, elapsedSeconds);

		pRenderTarget->DrawTextW(
			buff,
			length,
			pMenuTextFormat,
			D2D1::RectF(toPixels(32.0f), m_rect.bottom / 2 + toPixels(16.0f), m_rect.right - toPixels(32.0f), m_rect.bottom / 2 + toPixels(16.0f)),
			bBlack
		);
	}


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

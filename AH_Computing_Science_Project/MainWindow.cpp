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
		toPixels(48.0f),
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
		toPixels(24.0f),
		L"en-uk",
		&pMenuTextFormat
	);
	pMenuTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

	myGraphics->getWriteFactory()->CreateTextFormat(
		L"Ariel",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		toPixels(20.0f),
		L"en-uk",
		&pProfileTextFormat
	);
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
		m_logOutButton = CreateWindowEx(NULL, L"BUTTON", L"Change Account", WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, toPixels(32), toPixels(320), toPixels(224), toPixels(32), Window(), NULL, m_inst, NULL);
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
		lpMMI->ptMinTrackSize.x = toPixels(800);
		lpMMI->ptMinTrackSize.y = toPixels(736);
		break;
	}
	case CA_SHOWMAIN:
		ShowWindow(m_findGameButton, wParam);
		ShowWindow(m_logOutButton, wParam);
		ShowWindow(m_exitButton, wParam);

		if (wParam == SW_SHOW) {
			windowShown = true;
			if (lParam > 0) {
				m_userId = (*(AccountInfo*)lParam).userId;
				m_username = (*(AccountInfo*)lParam).username;

				network->recieveProfileData(&m_numberOfGames, &m_numberOfWins);

				OutputDebugStringA("Number of games: ");
				OutputDebugStringA(std::to_string(m_numberOfGames).c_str());
				OutputDebugStringA(".\n");
				OutputDebugStringA("Number of wins: ");
				OutputDebugStringA(std::to_string(m_numberOfWins).c_str());
				OutputDebugStringA(".\n");
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
								int res = network->checkForGame(m_userId);		
								if (res == 1) {
									SendMessage(m_hwnd, CA_SHOWMAIN, SW_SHOW, NULL);
									ShowWindow(m_cancelButton, SW_HIDE);
									findingGame = false;
								}
								else if (res == 2) {
									findingGame = false;
									ShowWindow(m_gameWindowHandle, SW_SHOW);
									SendMessage(m_gameWindowHandle, CA_SHOWGAME, SW_SHOW, (LPARAM) &m_userId);
								}								
								Sleep(1);
							}
						}
					}					
				}	
				else if ((HWND)lParam == m_logOutButton) {
					if (network->logOut()) {
						// Hide the main window
						SendMessage(m_hwnd, CA_SHOWMAIN, SW_HIDE, NULL);

						// Show the login window
						ShowWindow(m_logInHandle, SW_SHOW);
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

	if (windowShown || findingGame) {
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
		else {
			pRenderTarget->FillRectangle(D2D1::RectF(toPixels(m_rect.right / 3 * 2 + toPixels(32.0f)), m_rect.bottom - toPixels(148.0f), m_rect.right - toPixels(32.0f), m_rect.bottom - toPixels(32.0f)), bWhite);
			pRenderTarget->DrawRectangle(D2D1::RectF(toPixels(m_rect.right / 3 * 2 + toPixels(32.0f)), m_rect.bottom - toPixels(148.0f), m_rect.right - toPixels(32.0f), m_rect.bottom - toPixels(32.0f)), bBlack);

			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			std::wstring usernameText = L"Username: " + converter.from_bytes(m_username);
			std::wstring gamePlayedText = L"Games Played: " + std::to_wstring(m_numberOfGames);
			std::wstring gameWonText = L"Games Won: " + std::to_wstring(m_numberOfWins);

			pRenderTarget->DrawTextW(
				usernameText.c_str(),
				usernameText.length(),
				pProfileTextFormat,
				D2D1::RectF(toPixels(m_rect.right / 3 * 2 + toPixels(40.0f)), m_rect.bottom - toPixels(146.0f), m_rect.right - toPixels(32.0f), m_rect.bottom - toPixels(126.0f)),
				bBlack
			);

			pRenderTarget->DrawTextW(
				gamePlayedText.c_str(),
				gamePlayedText.length(),
				pProfileTextFormat,
				D2D1::RectF(toPixels(m_rect.right / 3 * 2 + toPixels(40.0f)), m_rect.bottom - toPixels(106.0f), m_rect.right - toPixels(32.0f), m_rect.bottom - toPixels(32.0f)),
				bBlack
			);

			pRenderTarget->DrawTextW(
				gameWonText.c_str(),
				gameWonText.length(),
				pProfileTextFormat,
				D2D1::RectF(toPixels(m_rect.right / 3 * 2 + toPixels(40.0f)), m_rect.bottom - toPixels(66.0f), m_rect.right - toPixels(32.0f), m_rect.bottom - toPixels(32.0f)),
				bBlack
			);
		}
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
		pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &bWhite);
	}
}

void MainWindow::discardGraphicResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&bBlack);
	SafeRelease(&bWhite);
}

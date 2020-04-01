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
		56.0f,
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

	myGraphics->getWriteFactory()->CreateTextFormat(
		L"Ariel",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		20.0f,
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
		m_findGameButton = CreateWindowEx(NULL, L"BUTTON", L"Find Game", WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0, Window(), NULL, m_inst, NULL);
		m_logOutButton = CreateWindowEx(NULL, L"BUTTON", L"Change Account", WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0, Window(), NULL, m_inst, NULL);
		m_exitButton = CreateWindowEx(NULL, L"BUTTON", L"Quit to Desktop", WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0, Window(), NULL, m_inst, NULL);

		// Finding Game Controls
		m_cancelButton = CreateWindowEx(NULL, L"BUTTON", L"Cancel", WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, m_rect.right / 2 - toPixels(150), m_rect.bottom - toPixels(32 + 48), toPixels(300), toPixels(48), Window(), NULL, m_inst, NULL);


		HFONT font = CreateFont(
			toPixels(24),
			0.0f,
			0.0f,
			0.0f,
			FW_MEDIUM,
			false,
			false,
			false,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			ANTIALIASED_QUALITY,
			DEFAULT_PITCH,
			L"Ariel"
		);

		SendMessage(m_findGameButton, WM_SETFONT, (WPARAM)font, MAKELPARAM(FALSE, 0));
		SendMessage(m_logOutButton, WM_SETFONT, (WPARAM)font, MAKELPARAM(FALSE, 0));
		SendMessage(m_exitButton, WM_SETFONT, (WPARAM)font, MAKELPARAM(FALSE, 0));
		SendMessage(m_cancelButton, WM_SETFONT, (WPARAM)font, MAKELPARAM(FALSE, 0));

		break;
	}
	case WM_DPICHANGED:
	{
		UINT dpi = HIWORD(wParam);
		DPIScale = dpi / 96.0f;
	}
	case WM_SIZE:
	{
		GetClientRect(m_hwnd, &m_rect);
		discardGraphicResources();
		InvalidateRect(m_hwnd, NULL, TRUE);

		m_dipRect = rectFromPix(D2D1::RectF(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom));

		if (m_logInHandle != NULL) {
			float margin = (m_rect.bottom - toPixels(672)) / 2;
			if (margin < toPixels(32)) margin = toPixels(32);
			MoveWindow(m_logInHandle, m_rect.right / 2 - toPixels(256), margin, toPixels(512), m_rect.bottom - 2 * margin, true);
		}

		if (m_gameWindowHandle != NULL) {
			MoveWindow(m_gameWindowHandle, 0, 0, m_rect.right, m_rect.bottom, true);
		}

		MoveWindow(m_findGameButton, m_rect.right / 32 + toPixels(8), toPixels(256), m_rect.right / 4 + toPixels(32), toPixels(32) + m_rect.bottom / 32, true);
		MoveWindow(m_logOutButton, m_rect.right / 32 + toPixels(8), toPixels(336) + m_rect.bottom / 16, m_rect.right / 4 + toPixels(32), toPixels(32) + m_rect.bottom / 32, true);
		MoveWindow(m_exitButton, m_rect.right / 32 + toPixels(8), toPixels(416) + m_rect.bottom / 8, m_rect.right / 4 + toPixels(32), toPixels(32) + m_rect.bottom / 32, true);

		MoveWindow(m_cancelButton, m_rect.right / 2 - toPixels(150), m_rect.bottom - toPixels(32 + 48), toPixels(300), toPixels(48), true);
		break;
	}
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

				SafeRelease(&m_usernameLayout);
				std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
				std::wstring usernameText = L"Username: " + converter.from_bytes(m_username);
				myGraphics->getWriteFactory()->CreateTextLayout(
					usernameText.c_str(),
					usernameText.size(),
					pProfileTextFormat,
					1024.0f,
					20.0f,
					&m_usernameLayout
				);

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
									ShowWindow(m_cancelButton, SW_HIDE);

									// This won't return until the game has finished.
									SendMessage(m_gameWindowHandle, CA_SHOWGAME, SW_SHOW, (LPARAM) &m_userId);
									// ---

									// By sending the userId and username again, the window will try to recieve the game information
									AccountInfo accInfo;
									accInfo.userId = m_userId;
									accInfo.username = m_username;
									SendMessage(m_hwnd, CA_SHOWMAIN, SW_SHOW, (LPARAM) &accInfo);
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
						SendMessage(m_logInHandle, CA_CLEARLOGIN, NULL, NULL);
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
			D2D1::RectF(32.0f, 32.0f, m_dipRect.right - 32.0f, 32.0f),
			bBlack
		);


		if (findingGame) {
			pRenderTarget->DrawTextW(
				L"searching for game...",
				_countof(L"searching for game..."),
				pMenuTextFormat,
				D2D1::RectF(32.0f, m_dipRect.bottom / 2 - 16.0f, m_dipRect.right - 32.0f, m_dipRect.bottom / 2 - 16.0f),
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
				D2D1::RectF(32.0f, m_dipRect.bottom / 2 + 16.0f, m_dipRect.right - 32.0f, m_dipRect.bottom / 2 + 16.0f),
				bBlack
			);
		}
		else {
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			//std::wstring usernameText = L"Username: " + converter.from_bytes(m_username);
			std::wstring gamePlayedText = L"Games Played: " + std::to_wstring(m_numberOfGames);
			std::wstring gameWonText = L"Games Won: " + std::to_wstring(m_numberOfWins);

			DWRITE_TEXT_METRICS metrics;
			m_usernameLayout->GetMetrics(&metrics);
			float boxWidth = metrics.width + 16.0f;
			if (boxWidth < 288.0f) boxWidth = 288.0f;

			pRenderTarget->FillRectangle(D2D1::RectF(m_dipRect.right - boxWidth - 32.0f, m_dipRect.bottom - 148.0f, m_dipRect.right - 32.0f, m_dipRect.bottom - 32.0f), bWhite);
			pRenderTarget->DrawRectangle(D2D1::RectF(m_dipRect.right - boxWidth - 32.0f, m_dipRect.bottom - 148.0f, m_dipRect.right - 32.0f, m_dipRect.bottom - 32.0f), bBlack);

			pRenderTarget->DrawTextLayout(
				D2D1::Point2F(m_dipRect.right - boxWidth - 24.0f, m_dipRect.bottom - 146.0f),
				m_usernameLayout,
				bBlack
			);

			pRenderTarget->DrawTextW(
				gamePlayedText.c_str(),
				gamePlayedText.length(),
				pProfileTextFormat,
				D2D1::RectF(m_dipRect.right - boxWidth - 24.0f, m_dipRect.bottom - 106.0f, m_dipRect.right - 32.0f, m_dipRect.bottom - 32.0f),
				bBlack
			);

			pRenderTarget->DrawTextW(
				gameWonText.c_str(),
				gameWonText.length(),
				pProfileTextFormat,
				D2D1::RectF(m_dipRect.right - boxWidth - 24.0f, m_dipRect.bottom - 66.0f, m_dipRect.right - 32.0f, m_dipRect.bottom - 32.0f),
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

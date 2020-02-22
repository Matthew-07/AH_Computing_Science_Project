#include "LogInWindow.h"
#include "Graphics.h"

PCWSTR LogInWindow::ClassName() const
{
	return (PCWSTR)m_className;
}

LogInWindow::LogInWindow(Graphics* graphics, Network* nw, HWND parentHandle) : bBlack(NULL), m_loginButton(NULL), m_usernameEdit(NULL), m_passwordEdit(NULL) {
	LoadStringW(m_inst, IDS_LOGINWINDOWNAME, m_className, MAX_LOADSTRING);

	m_parentHwnd = parentHandle;

	myGraphics = graphics;
	network = nw;
	m_rect = RECT();

	myGraphics->getWriteFactory()->CreateTextFormat(
		L"Ariel",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		toPixels(24.0f),
		L"en-uk",
		&pLoginTextFormat
	);

	myGraphics->getWriteFactory()->CreateTextFormat(
		L"Ariel",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		toPixels(16.0f),
		L"en-uk",
		&pErrorTextFormat
	);

	myGraphics->getWriteFactory()->CreateTextFormat(
		L"Ariel",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		toPixels(36.0f),
		L"en-uk",
		&pHeadingTextFormat
	);
	pHeadingTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
}

LRESULT LogInWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
	{
		UINT dpi = GetDpiForWindow(m_hwnd);
		DPIScale = dpi / 96.0f;

		m_usernameEdit = CreateWindowEx(NULL, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, toPixels(16), toPixels(156), toPixels(480), toPixels(32), Window(), NULL, m_inst, NULL);
		PostMessage(m_usernameEdit, EM_LIMITTEXT, (WPARAM)32, NULL); // Set max length to 32
		m_passwordEdit = CreateWindowEx(NULL, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_PASSWORD, toPixels(16), toPixels(244), toPixels(480), toPixels(32), Window(), NULL, m_inst, NULL);
		PostMessage(m_passwordEdit, EM_LIMITTEXT, (WPARAM)32, NULL); // Set max length to 32
		m_confirmEdit = CreateWindowEx(NULL, L"EDIT", L"", WS_CHILD | WS_TABSTOP | WS_BORDER | ES_PASSWORD, toPixels(16), toPixels(332), toPixels(480), toPixels(32), Window(), NULL, m_inst, NULL);
		PostMessage(m_confirmEdit, EM_LIMITTEXT, (WPARAM)32, NULL); // Set max length to 32
		m_switchModeButton = CreateWindowEx(NULL, L"BUTTON", L"I don't have an account.", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, toPixels(16), toPixels(380), toPixels(224), toPixels(32), Window(), NULL, m_inst, NULL);
		m_loginButton = CreateWindowEx(NULL, L"BUTTON", L"Login", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, toPixels(272), toPixels(380), toPixels(224), toPixels(32), Window(), NULL, m_inst, NULL);

		break;
	}
	case WM_DPICHANGED:
	{
		UINT dpi = GetDpiForWindow(m_hwnd);

		MoveWindow(m_usernameEdit, toPixels(16), toPixels(156), toPixels(480), toPixels(32) ,false);
		MoveWindow(m_passwordEdit, toPixels(16), toPixels(244), toPixels(480), toPixels(32), false);
		MoveWindow(m_confirmEdit, toPixels(16), toPixels(332), toPixels(480), toPixels(32), false);
		MoveWindow(m_switchModeButton, toPixels(16), toPixels(380), toPixels(224), toPixels(32), false);
		MoveWindow(m_loginButton, toPixels(272), toPixels(380), toPixels(224), toPixels(32), false);

		DPIScale = dpi / 96.0f;
		InvalidateRect(m_hwnd, NULL, false);
		break;
	}
	case WM_SIZE:
		GetClientRect(m_hwnd, &m_rect);
		discardGraphicResources();
		InvalidateRect(m_hwnd, NULL, TRUE);
		break;
	case WM_PAINT:
		onPaint();
		break;
	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
			if ((HWND)lParam == m_loginButton) {
				int usernameLength = GetWindowTextLength(m_usernameEdit) + 1;
				// <= because the value in usernameLength includes the NULL character.
				if (usernameLength <= 4) {
					m_errorText = L"Username must be atleast 4 characters.";
					InvalidateRect(m_hwnd, NULL, FALSE);
					break;
				}
				CHAR* username = new CHAR[usernameLength];		

				int passwordLength = GetWindowTextLength(m_passwordEdit) + 1;
				if (passwordLength <= 8) {
					m_errorText = L"Password must be at least 8 characters.";
					InvalidateRect(m_hwnd, NULL, FALSE);
					break;
				}
				CHAR* password = new CHAR[passwordLength];

				GetWindowTextA(m_usernameEdit, username, usernameLength);
				std::string usernameStr(username);

				GetWindowTextA(m_passwordEdit, password, passwordLength);
				std::string passwordStr(password);

				int id;

				if (newAccountMode) {
					/* Same as when false but there will be two password edits which must be equal to eachother
					and the first parameter of network->LogIn(...) will be true.*/
					int confirmLength = GetWindowTextLength(m_confirmEdit) + 1;
					if (confirmLength <= 8) {
						m_errorText = L"Passwords do not match.";
						InvalidateRect(m_hwnd, NULL, FALSE);
						break;
					}
					CHAR* confirm = new CHAR[passwordLength];

					GetWindowTextA(m_confirmEdit, confirm, confirmLength);
					std::string confirmStr(confirm);

					if (passwordStr != confirmStr) {
						m_errorText = L"Passwords do not match.";
						InvalidateRect(m_hwnd, NULL, FALSE);
						break;
					}
					delete[] confirm;

					id = network->logIn(true, usernameStr, passwordStr);
				}
				else {
					id = network->logIn(false, usernameStr, passwordStr);
				}

				OutputDebugStringA(std::to_string(id).c_str());
				OutputDebugStringA("\n");

				delete[] username, password;

				if (id > 0) {
					// Login successful
					SendMessage(m_parentHwnd, CA_SHOWMAIN, SW_SHOW, NULL);
					ShowWindow(Window(), SW_HIDE);			
				}
				else {
					if (newAccountMode) {
						// For now assume this was the cause of the failure.
						m_errorText = L"That username is already in use.";
						InvalidateRect(m_hwnd, NULL, false);
					}
					else {
						// Again assume there were no issues in the communication with the game coordinator.
						m_errorText = L"Username or password incorrect.";
						InvalidateRect(m_hwnd, NULL, false);
					}
				}

			}
			else if ((HWND)lParam == m_switchModeButton) {
				if (newAccountMode) {
					ShowWindow(m_confirmEdit, SW_HIDE);
					// Clear control text
					textChangedByProgram = true;
					SetWindowText(m_usernameEdit,L"");
					SetWindowText(m_passwordEdit, L"");

					SetWindowText(m_switchModeButton, L"I don't have an account.");
					SetWindowText(m_loginButton, L"Login");
					textChangedByProgram = false;

					newAccountMode = false;
					InvalidateRect(m_hwnd, NULL, false);
				}
				else {
					ShowWindow(m_confirmEdit, SW_SHOW);
					newAccountMode = true;

					// Clear control text
					textChangedByProgram = true;
					SetWindowText(m_usernameEdit, L"");
					SetWindowText(m_passwordEdit, L"");
					SetWindowText(m_confirmEdit, L"");

					SetWindowText(m_switchModeButton, L"I already have an account.");
					SetWindowText(m_loginButton, L"Create Account");
					textChangedByProgram = false;
					InvalidateRect(m_hwnd, NULL, false);
				}
			}
			break;

		case EN_UPDATE:
			if (!textChangedByProgram) {
				int length = GetWindowTextLengthA((HWND)lParam) + 1;
				char* text = new char[length];
				GetWindowTextA((HWND)lParam, text, length);
				DWORD pos;
				SendMessageA((HWND)lParam, EM_GETSEL,(WPARAM) &pos, NULL);

				std::string str(text);

				for (int i = str.length(); i >= 0; i--) {
					int val = str[i];
					if (!(36 <= val && val <= 57 || 65 <= val && val <= 90 || 97 <= val && val <= 122 || val && val == 95) || val == 39 || val ==  45 || val == 37){
						str.erase(i, 1);
					}
				}

				delete[] text;

				textChangedByProgram = true;
				SetWindowTextA((HWND)lParam, str.c_str());
				textChangedByProgram = false;
				PostMessageA((HWND)lParam, EM_SETSEL, (WPARAM)pos, (LPARAM)pos + 1);
				break;
			}
		}
		break;
	}

	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void LogInWindow::onPaint() {
	createGraphicResources();

	// Begin Draw
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hwnd, &ps);

	pRenderTarget->BeginDraw();

	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF(1.0f, 1.0f, 1.0f)));

	// Draw Window
	pRenderTarget->DrawRectangle(rectToPix(D2D1::RectF(m_rect.left,m_rect.top,m_rect.right,m_rect.bottom)),bBlack);

	pRenderTarget->DrawTextW(
		L"Username: ",
		(UINT32) 10,
		pLoginTextFormat,
		rectToPix(D2D1::RectF(16.0f,116.0f,480.0f,116.0f)),
		bBlack
	);

	pRenderTarget->DrawTextW(
		L"Password: ",
		(UINT32)10,
		pLoginTextFormat,
		rectToPix(D2D1::RectF(16.0f, 204.0f, 480.0f, 204.0f)),
		bBlack
	);

	// Draw error text
	pRenderTarget->DrawTextW(
		m_errorText.c_str(),
		m_errorText.length(),
		pErrorTextFormat,
		rectToPix(D2D1::RectF(16.0f, 444.0f, 480.0f, 444.0f)),
		bRed
	);

	const wchar_t* headerText;
	int length;

	if (newAccountMode) {
		pRenderTarget->DrawTextW(
			L"Confirm Password: ",
			(UINT32)19,
			pLoginTextFormat,
			rectToPix(D2D1::RectF(16.0f, 292.0f, 480.0f, 292.0f)),
			bBlack
		);

		headerText = L"Create Account";
		length = 14;
	}
	else
	{
		headerText = L"Login";
		length = 5;
	}

	pRenderTarget->DrawTextW(
		headerText,
		(UINT32)length,
		pHeadingTextFormat,
		rectToPix(D2D1::RectF(16.0f, 16.0f, 496.0f, 16.0f)),
		bBlack
	);


	//End Draw
	pRenderTarget->EndDraw();

	EndPaint(m_hwnd, &ps);
}

void LogInWindow::createGraphicResources()
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
		pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &bRed);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.2f), &pExplosionBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green), &pFoodBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &pBorderBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.5f, 1.0f, 0.5f, 0.15f), &pSightRangeBrush);
	}
}

void LogInWindow::discardGraphicResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&bBlack);
}

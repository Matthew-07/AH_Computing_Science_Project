#include "LogInWindow.h"
#include "Graphics.h"

PCWSTR LogInWindow::ClassName() const
{
	return (PCWSTR)m_className;
}

LogInWindow::LogInWindow(Graphics* graphics, Network* nw){
	LoadStringW(m_inst, IDS_LOGINWINDOWNAME, m_className, MAX_LOADSTRING);

	myGraphics = graphics;
	network = nw;
	m_rect = RECT();
}

LRESULT LogInWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		m_usernameEdit = CreateWindowEx(NULL, L"EDIT", L"<Enter Username>", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, 16, 16, 256, 24, Window(), NULL, m_inst, NULL);
		PostMessage(m_usernameEdit, EM_LIMITTEXT, (WPARAM) 32, NULL); // Set max length to 32
		m_passwordEdit = CreateWindowEx(NULL, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_PASSWORD, 16, 56, 256, 24, Window(), NULL, m_inst, NULL);
		PostMessage(m_passwordEdit, EM_LIMITTEXT, (WPARAM)32, NULL); // Set max length to 32
		m_loginButton = CreateWindowEx(NULL, L"BUTTON", L"Login", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 16, 96, 256, 24, Window(), NULL, m_inst, NULL);
		break;
	case WM_SIZE:
		GetClientRect(m_hwnd, &m_rect);
		discardGraphicResources();
		InvalidateRect(m_hwnd, NULL, TRUE);
		break;
	case WM_PAINT:
		onPaint();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
			if ((HWND)lParam == m_loginButton) {
				int usernameLength = GetWindowTextLength(m_usernameEdit) + 1;
				if (usernameLength < 4) break;
				CHAR* username = new CHAR[usernameLength];

				int passwordLength = GetWindowTextLength(m_passwordEdit) + 1;
				CHAR* password = new CHAR[passwordLength];
				if (passwordLength < 8) break;

				GetWindowTextA(m_usernameEdit, username, usernameLength);
				GetWindowTextA(m_passwordEdit, password, passwordLength);
				int id = network->logIn(false, std::string(username), std::string(password));

				OutputDebugStringA(std::to_string(id).c_str());
				OutputDebugStringA("\n");

				delete[] username, password;
				break;
			}
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
	pRenderTarget->DrawRectangle(D2D1::RectF(m_rect.left,m_rect.top,m_rect.right,m_rect.bottom),bBlack);


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
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBombBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.2f), &pExplosionBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green), &pFoodBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &pBorderBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.5f, 1.0f, 0.5f, 0.15f), &pSightRangeBrush);
	}
}

void LogInWindow::discardGraphicResources()
{
	SafeRelease(&pRenderTarget);
}

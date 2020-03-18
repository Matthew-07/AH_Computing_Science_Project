#pragma once
#include "BaseWindow.h"
#include "CustomMessages.h"

class Graphics;
class Network;

class LogInWindow :
	public BaseWindow<LogInWindow>
{
public:
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LogInWindow(Graphics* graphics, Network* nw, HWND parentHandle);

private:
	HWND m_parentHwnd;

	PCWSTR ClassName() const;
	WCHAR m_className[MAX_LOADSTRING];

	void onPaint();
	void createGraphicResources();
	void discardGraphicResources();

	void createDWriteResources();

	ID2D1HwndRenderTarget* pRenderTarget;
	ID2D1SolidColorBrush *bBlack, *bRed;

	IDWriteTextFormat* pLoginTextFormat;
	IDWriteTextFormat* pErrorTextFormat;
	IDWriteTextFormat* pHeadingTextFormat;

	std::wstring m_errorText = L"";

	RECT m_rect;

	Graphics* myGraphics = NULL;
	Network* network;

	// Login
	HWND m_usernameEdit, m_passwordEdit, m_confirmEdit, m_loginButton, m_switchModeButton;
	bool textChangedByProgram = false;
	bool newAccountMode = false;
};


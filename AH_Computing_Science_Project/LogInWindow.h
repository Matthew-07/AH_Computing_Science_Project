#pragma once
#include "BaseWindow.h"

class Graphics;
class Network;

class LogInWindow :
	public BaseWindow<LogInWindow>
{
public:
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LogInWindow(Graphics* graphics, Network* nw);

private:
	PCWSTR ClassName() const;
	WCHAR m_className[MAX_LOADSTRING];

	void onPaint();
	void createGraphicResources();
	void discardGraphicResources();

	ID2D1HwndRenderTarget* pRenderTarget;
	ID2D1SolidColorBrush *bBlack;

	RECT m_rect;

	Graphics* myGraphics = NULL;
	Network* network;

	// Login
	HWND m_usernameEdit, m_passwordEdit, m_loginButton;
	bool textChangedByProgram = false;
};


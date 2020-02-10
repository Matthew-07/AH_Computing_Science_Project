#pragma once

#include "BaseWindow.h"
#include "CustomMessages.h"

// Tell compiler that classes exist
class Graphics;
class Network;

class MainWindow:
	public BaseWindow<MainWindow>
{
public:
	MainWindow(Graphics * graphics, Network* nw);
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void SetLogInHandle(HWND hWnd) { m_logInHandle = hWnd; }
private:
	PCWSTR ClassName() const;
	WCHAR m_className[MAX_LOADSTRING];

	HWND m_logInHandle = NULL;

	void onPaint();
	void createGraphicResources();
	void discardGraphicResources();

	ID2D1HwndRenderTarget *pRenderTarget;
	ID2D1SolidColorBrush *bBlack;
	IDWriteTextFormat *pTitleTextFormat;

	RECT m_rect;

	Graphics *myGraphics = NULL;
	Network* network = NULL;
	bool windowShown = false;

	HWND m_findGameButton, m_settingsButton, m_exitButton;
};


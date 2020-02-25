#pragma once

#include "BaseWindow.h"

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

	void setUserId(int32_t& id) {
		m_userId = id;
	}
private:
	PCWSTR ClassName() const;
	WCHAR m_className[MAX_LOADSTRING];

	HWND m_logInHandle = NULL;

	void onPaint();
	void createGraphicResources();
	void discardGraphicResources();

	ID2D1HwndRenderTarget *pRenderTarget;
	ID2D1SolidColorBrush *bBlack;
	IDWriteTextFormat *pTitleTextFormat, *pMenuTextFormat;

	RECT m_rect;

	Graphics *myGraphics = NULL;
	Network* network = NULL;
	bool windowShown = false;

	HWND m_findGameButton, m_settingsButton, m_exitButton;
	HWND m_cancelButton;

	bool findingGame = false;
	std::chrono::steady_clock::time_point findGameTimer;

	int32_t m_userId;

};


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
	void SetGameWindowHandle(HWND hWnd) { m_gameWindowHandle = hWnd; }

	void setUserId(int32_t& id) {
		m_userId = id;
	}
private:
	PCWSTR ClassName() const;
	WCHAR m_className[MAX_LOADSTRING];

	HWND m_logInHandle = NULL, m_gameWindowHandle = NULL;

	void onPaint();
	void createGraphicResources();
	void discardGraphicResources();

	ID2D1HwndRenderTarget *pRenderTarget;
	ID2D1SolidColorBrush *bBlack, *bWhite;
	IDWriteTextFormat *pTitleTextFormat, *pMenuTextFormat, * pProfileTextFormat;

	RECT m_rect;
	D2D1_RECT_F m_dipRect;

	Graphics *myGraphics = NULL;
	Network* network = NULL;
	bool windowShown = false;

	HWND m_findGameButton, m_logOutButton, m_exitButton;
	HWND m_cancelButton;

	bool findingGame = false;
	std::chrono::steady_clock::time_point findGameTimer;

	int32_t m_userId = -1;
	std::string m_username;
	IDWriteTextLayout *m_usernameLayout;

	int32_t m_numberOfGames;
	int32_t m_numberOfWins;

};


#pragma once

#include "CA_pch.h"
#include "resource.h"
#include "Network.h"

// Tell compiler that the window classes exists
class MainWindow;
class LogInWindow;

// Class for handling graphics
class Graphics {
public:
	Graphics(Network * nw);
	~Graphics();
	bool init(HINSTANCE instance, int nCmdShow);

	ID2D1Factory * getFactory() { return pFactory; }
	IDWriteFactory* getWriteFactory() { return pWriteFactory; }

	HWND getMainHWND();

	bool fastRedraw = false;

private:
	Network* network = NULL;

	MainWindow * w_main = NULL;
	LogInWindow* w_logIn = NULL;

	HINSTANCE m_inst;

	bool createMainWindow(int nCmdShow);

	WCHAR m_appName[MAX_LOADSTRING];

	ID2D1Factory* pFactory;
	IDWriteFactory* pWriteFactory;	
};
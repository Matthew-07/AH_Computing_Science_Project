#pragma once

#include "CA_pch.h"
#include "resource.h"

// Tell compiler "MainWindow" class exists
class MainWindow;

// Class for handling graphics
class Graphics {
public:
	Graphics();
	~Graphics();
	bool init(HINSTANCE instance, int nCmdShow);

	ID2D1Factory * getFactory() { return pFactory; }

private:

	MainWindow * w_main = NULL;

	HINSTANCE m_inst;

	bool createMainWindow(int nCmdShow);

	WCHAR m_appName[MAX_LOADSTRING];

	ID2D1Factory* pFactory;
};
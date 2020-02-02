#pragma once

#include "CA_pch.h"
#include "resource.h"

#include "MainWindow.h"


// Class for handling graphics
class Graphics {
public:
	Graphics();
	bool init(HINSTANCE instance, int nCmdShow);

private:

	MainWindow w_main;

	HINSTANCE m_inst;

	bool createMainWindow(int nCmdShow);

	WCHAR m_appName[MAX_LOADSTRING];

	ID2D1Factory* pFactory;
};
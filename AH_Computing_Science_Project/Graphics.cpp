#include "Graphics.h"

Graphics::Graphics() : w_main(), pFactory(NULL), m_appName(L"") {
	m_inst = NULL;
}

bool Graphics::init(HINSTANCE instance, int nCmdShow) {
	m_inst = instance;

	LoadStringW(m_inst, IDS_APPNAME, m_appName, MAX_LOADSTRING);

	if (!createMainWindow(nCmdShow)) {
		return false;
	}

	return true;
}

bool Graphics::createMainWindow(int nCmdShow) {
	
	if (!w_main.Create(L"Main Window", WS_OVERLAPPEDWINDOW)) {
		return false;
	}

	ShowWindow(w_main.Window(), nCmdShow);

	return true;
}
#include "Graphics.h"
#include "MainWindow.h"

Graphics::Graphics() : pFactory(NULL), m_appName(L"") {
	m_inst = NULL;

	w_main = new MainWindow(this);
}

Graphics::~Graphics() {
	delete w_main;
}

bool Graphics::init(HINSTANCE instance, int nCmdShow) {
	m_inst = instance;

	LoadStringW(m_inst, IDS_APPNAME, m_appName, MAX_LOADSTRING);

	if (!createMainWindow(nCmdShow)) {
		return false;
	}

	if (FAILED(D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
	{
		return false;
	}

	return true;
}

bool Graphics::createMainWindow(int nCmdShow) {
	
	if (!w_main->Create(L"Reflex", WS_OVERLAPPEDWINDOW)) {
		//LPVOID lpMsgBuf;
		//LPVOID lpDisplayBuf;
		//DWORD dw = GetLastError();

		//FormatMessage(
		//	FORMAT_MESSAGE_ALLOCATE_BUFFER |
		//	FORMAT_MESSAGE_FROM_SYSTEM |
		//	FORMAT_MESSAGE_IGNORE_INSERTS,
		//	NULL,
		//	dw,
		//	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		//	(LPTSTR)&lpMsgBuf,
		//	0, NULL);

		//// Display the error message and exit the process

		//MessageBox(NULL, (LPCTSTR)lpMsgBuf, TEXT("Error"), MB_OK);

		//LocalFree(lpMsgBuf);
		////LocalFree(lpDisplayBuf);
		//ExitProcess(dw);
		return false;
	}

	ShowWindow(w_main->Window(), nCmdShow);

	return true;
}
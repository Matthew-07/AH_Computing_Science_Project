#include "Graphics.h"
#include "MainWindow.h"
#include "LogInWindow.h"

Graphics::Graphics(Network * nw) : pFactory(NULL), m_appName(L"") {
	m_inst = NULL;

	network = nw;	
}

Graphics::~Graphics() {
	delete w_main;
	delete w_logIn;
}

HWND Graphics::getMainHWND() { return w_main->Window(); }

bool Graphics::init(HINSTANCE instance, int nCmdShow) {
	m_inst = instance;

	if (FAILED(D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
	{
		return false;
	}
	if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&pWriteFactory)
	))) {
		return false;
	}	

	LoadStringW(m_inst, IDS_APPNAME, m_appName, MAX_LOADSTRING);

	if (!createMainWindow(nCmdShow)) {
		return false;
	}	

	return true;
}

bool Graphics::createMainWindow(int nCmdShow) {
	w_main = new MainWindow(this, network);
	
	if (!w_main->Create(L"Reflex", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN)) {

		return false;
	}

	RECT winRect;
	GetClientRect(w_main->Window(), &winRect);

	w_logIn = new LogInWindow(this, network, w_main->Window());

	if (!w_logIn->Create(L"LOGIN", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0UL, 0, 0, 0, 0, w_main->Window())) {
		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		DWORD dw = GetLastError();

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);

		// Display the error message and exit the process

		MessageBox(NULL, (LPCTSTR)lpMsgBuf, TEXT("Error"), MB_OK);

		LocalFree(lpMsgBuf);
		//LocalFree(lpDisplayBuf);
		ExitProcess(dw);
		return false;
	}

	w_main->SetLogInHandle(w_logIn->Window());

	ShowWindow(w_main->Window(), nCmdShow);

	return true;
}
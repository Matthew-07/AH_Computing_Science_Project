#include "Graphics.h"
#include "MainWindow.h"
#include "LogInWindow.h"

Graphics::Graphics(Network * nw) : pFactory(NULL), m_appName(L"") {
	m_inst = NULL;

	network = nw;

	w_main = new MainWindow(this);
	w_logIn = new LogInWindow(this, nw);
}

Graphics::~Graphics() {
	delete w_main;
	delete w_logIn;
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
	
	if (!w_main->Create(L"Reflex", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN)) {
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

	RECT winRect;
	GetClientRect(w_main->Window(), &winRect);

	if (!w_logIn->Create(L"LOGIN", WS_CHILD | WS_VISIBLE, 0UL, 0, 0, 0, 0, w_main->Window())) {
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
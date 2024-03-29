#include "CA_pch.h"
#include "Graphics.h"
#include "Network.h"

// Main function for windows application
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{

	// Connect to server
	Network network = Network();
	if (!network.init()) {
		return -1;
	}

	// Initate Graphics
	Graphics myGraphics = Graphics(&network);

	if (!myGraphics.init(hInstance, nCmdShow)) {
		return -1;
	}

	MSG msg = { };

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;

}
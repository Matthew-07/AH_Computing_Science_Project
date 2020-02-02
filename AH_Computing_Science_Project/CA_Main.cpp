#include "CA_pch.h"
#include "Graphics.h"

// Main function for windows application
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{

	// Connect to server

	// Initate Graphics
	Graphics myGraphics = Graphics();

	if (!myGraphics.init(hInstance, nCmdShow)) {
		return -1;
	}

	// User logs in

	//Main Menu - user can search for a game or view their statistics.

	MSG msg = { };

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;

}
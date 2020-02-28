#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define MAX_LOADSTRING 100

#ifndef UNICODE
#define UNICODE
#endif

#include <winsock2.h>

#include <windows.h>

#include <d2d1.h>
#include <dwrite.h>
#pragma comment(lib,"d2d1")
#pragma comment(lib,"dwrite")

#include <stdlib.h>
#include <chrono>
#include <thread>

#include <list>
#include <vector>

#include <ws2tcpip.h>

#include "../Game Coordinator/WinsockHelper.h"

#include <stdio.h>

#include <iostream>
#include <fstream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

template <class T> void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}
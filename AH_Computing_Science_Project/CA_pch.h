#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define MAX_LOADSTRING 100

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>

#include<d2d1.h>
#pragma comment(lib,"d2d1")

#include <stdlib.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

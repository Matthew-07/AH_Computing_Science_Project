#pragma once
#include "GS_pch.h"

class Server {
public:
	bool init();
	bool start();
private:
	bool senddata(SOCKET sock, void* buf, int buflen);
	int MAXGAMES = 1;
	std::string MAXGAMESMSG;
};
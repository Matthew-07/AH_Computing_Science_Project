#pragma once
#include "GS_pch.h"

class Server {
public:
	bool init();
	bool start();
private:
	int MAXGAMES = 1;
	std::string MAXGAMESMSG;
};
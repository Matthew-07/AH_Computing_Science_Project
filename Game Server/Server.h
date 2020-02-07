#pragma once
#include "GS_pch.h"

#define COORDINATOR_PORT "26534"

class Server {
public:
	bool init();
	bool start();
private:
	int MAXGAMES = 1;
	std::string MAXGAMESMSG;
};
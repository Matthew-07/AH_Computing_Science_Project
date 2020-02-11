#pragma once
#include "GS_pch.h"

#define COORDINATOR_PORT "26534"
#define CLIENT_PORT 26533

//#define BUFFER_SIZE 512
#define BUFFER_SIZE 8

class Server {
public:
	bool init();
	bool start();
	bool recievePackets();
private:
	WSADATA wsaData;

	//int MAXGAMES = 1;
	//std::string MAXGAMESMSG;

	std::thread * m_recievePacketsThread;

	in_addr6 m_ip;
};
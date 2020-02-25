#pragma once
#include "GS_pch.h"
#include "Logic.h"

#define COORDINATOR_PORT "26534"
#define CLIENT_PORT 26533
#define CLIENT_PORT_STRING "26533"

//#define BUFFER_SIZE 512
#define BUFFER_SIZE 8

class Server {
public:
	bool init();
	bool start();
	bool recievePackets();

	bool gameThread(Logic & game);

	bool userConnectionsThread();
	bool userThread(LPVOID clientSocket);
private:
	WSADATA wsaData;

	SOCKET m_GCsocket, m_UserListenSocket;

	//int MAXGAMES = 1;
	//std::string MAXGAMESMSG;

	std::thread* m_recievePacketsThread;

	std::vector<std::thread>	m_gameThreads;

	std::thread* m_userConnectionsThread;
	std::vector<std::thread>	m_userThreads;
	

	in_addr6 m_ip;
};
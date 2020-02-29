#pragma once
#include "GS_pch.h"
#include "Logic.h"

#define COORDINATOR_PORT "26534"
#define CLIENT_PORT 26533
#define CLIENT_PORT_STRING "26533"

#define GAME_PORT 26536

//#define BUFFER_SIZE 512
#define BUFFER_SIZE 8

struct ConnectedPlayer {
	SOCKET socket;
	int32_t userId;

	sockaddr_in6 address;
};

struct Game {
	Logic *logic;
	std::list <ConnectedPlayer*> players;
	std::list <Input>	playerInputs;
	bool gameEnded = false;
};

class Server {
public:
	bool init();
	bool start();
	bool recievePackets();

	bool gameThread(Game* game);

	bool userConnectionsThread();
	bool userThread(LPVOID clientSocket);
private:
	WSADATA wsaData;

	SOCKET m_GCsocket, m_UserListenSocket;

	//int MAXGAMES = 1;
	//std::string MAXGAMESMSG;

	std::thread* m_recievePacketsThread;

	std::list<std::thread>	m_gameThreads;
	std::list<Game*>		m_games;

	std::thread* m_userConnectionsThread;
	std::list<std::thread>	m_userThreads;
	

	in_addr6 m_ip;
};
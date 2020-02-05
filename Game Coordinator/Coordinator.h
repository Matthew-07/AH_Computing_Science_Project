#pragma once

#include "GC_pch.h"

class Database;

class Coordinator {
public:
	Coordinator(Database* db);

	bool init(); // Initalize winsock
	bool startServer(); // Prepare game coordinator
	bool run(); // Run the game coordinator

private:
	bool userConnectionsThread(); // Thread for accepting client connections
	bool gameServerConnectionsThread(); // Thread for accepting game server connections
	bool userThread(LPVOID clientSocket); // Thread for handling an individual client
	bool gameServerThread(LPVOID serverSocket); // Thread for handling an individual game server

#define GAMESERVER_PORT "56534"
#define USER_PORT "56535"
	//const char* HOSTNAME = "mae_ahcompsci_gc";

	SOCKET m_UserListenSocket;
	SOCKET m_GameServerListenSocket;

	std::thread*				m_userConnectionsThread;
	std::thread*				m_serverConnectionsThread;
	std::vector<std::thread>	m_userThreads;
	std::vector<std::thread>	m_serverThreads;

	Database *m_db = NULL;
};
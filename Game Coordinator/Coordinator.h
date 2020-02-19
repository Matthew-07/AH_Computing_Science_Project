#pragma once

#include "GC_pch.h"

// Used for storing information about the latency between a player and server.
struct Connection {
public:
	in6_addr serverIP;
	float ping;

	Connection(in6_addr id, int32_t p) {
		serverIP = id;
		ping = p;
	}
};

// Store information about each player in queue to find a game
struct Player {
public:
	int id = -1;
	std::vector<Connection> pings;
	SOCKET* playerSocket; // So the matchmaking thread can send info about the game.

	void addConnection(in6_addr ip, int32_t ping) {
		pings.push_back(Connection(ip, ping));
	}

	bool shouldLeave = false;

	//std::list<int32_t>* threadTasks;
	//void* info;
};

struct Server {
	in_addr6* ip;
	SOCKET* socket;
};

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

	bool matchmakingThread(); // Thread for assigning players to games.

#define GAMESERVER_PORT "26534"
#define USER_PORT "26535"
	//const char* HOSTNAME = "mae_ahcompsci_gc";

	SOCKET m_UserListenSocket;
	SOCKET m_GameServerListenSocket;

	std::thread*				m_userConnectionsThread;
	std::thread*				m_serverConnectionsThread;
	std::thread*				m_matchmakingThread;
	std::vector<std::thread>	m_userThreads;
	std::vector<std::thread>	m_serverThreads;

	std::list<Player>			m_matchmakingQueue;

	std::list<Server>			m_servers;

	Database *m_db = NULL;
};
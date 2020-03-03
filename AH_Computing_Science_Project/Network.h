#pragma once

#include "CA_pch.h"

#define COORDINATOR_PORT "26535"
#define GAMESERVER_PORT	"26533"

struct Input;

class Network
{
public:
	bool init(); // Initialize Winsock
	bool startConnection(); // Connect to game coordinator
	int logIn(bool newAccount, std::string username, std::string password);
	bool joinMatchmakingQueue();
	bool leaveMatchmakingQueue();
	bool checkForGame(int32_t& userId);

	bool getGameInfo(int32_t *numberOfPlayers, int32_t *numberOfTeams, int32_t** playerIds, int32_t** playerTeams, int32_t* maxGamestateSize);
	bool recievePacket(char * buffer);

	bool sendInput(Input* i);

private:
	SOCKET m_GCSocket, m_udpSocket;	
	int32_t udpPort;

	bool joinGame(in6_addr* serverAddress);
	void checkPings(in6_addr * addressBuffer, int64_t * avgPingBuffer, int numberOfServers);
	void sendPings(SOCKET s, sockaddr_in6 * addr);
	void recievePings(SOCKET s, int64_t** pingBuffer, in6_addr* addressBuffer, int numberOfServers);

	bool inQueue = false;

	int32_t m_userId;

	sockaddr_in6 m_serverAddr;
	SOCKET m_tcpServerSocket = INVALID_SOCKET;

	int32_t maxSize;
};


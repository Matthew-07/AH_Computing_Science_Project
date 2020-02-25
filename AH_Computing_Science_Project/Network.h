#pragma once

#include "CA_pch.h"

#define COORDINATOR_PORT "26535"
#define GAMESERVER_PORT	"26533"

class Network
{
public:
	bool init(); // Initialize Winsock
	bool startConnection(); // Connect to game coordinator
	int logIn(bool newAccount, std::string username, std::string password);
	bool joinMatchmakingQueue();
	bool leaveMatchmakingQueue();
	bool checkForGame(int32_t& userId);

private:
	SOCKET m_GCSocket, m_udpSocket;	
	bool joinGame(in6_addr* serverAddress);
	void checkPings(in6_addr * addressBuffer, int64_t * avgPingBuffer, int numberOfServers);
	void sendPings(SOCKET s, sockaddr_in6 * addr);
	void recievePings(SOCKET s, int64_t** pingBuffer, in6_addr* addressBuffer, int numberOfServers);

	bool inQueue = false;

	int32_t m_userId;
};


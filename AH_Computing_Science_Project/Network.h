#pragma once

#include "CA_pch.h"

#define COORDINATOR_PORT "26535"

class Network
{
public:
	bool init(); // Initialize Winsock and connect to game coordinator
	int logIn(bool newAccount, std::string username, std::string password);
	bool joinMatchmakingQueue();
private:
	SOCKET m_GCSocket;
	SOCKET m_udpSocket;

	void checkPings(in6_addr * addressBuffer, int64_t * avgPingBuffer, int numberOfServers);
	void sendPings(sockaddr* address, int slen);
	void recievePings(sockaddr* address, int64_t* pingBuffer, int slen);
};


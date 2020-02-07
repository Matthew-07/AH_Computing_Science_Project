#pragma once

#include "CA_pch.h"

#define COORDINATOR_PORT "26535"

class Network
{
public:
	bool init(); // Initialize Winsock and connect to game coordinator
	int logIn(bool newAccount, std::string username, std::string password);
private:
	SOCKET m_GCSocket;

};


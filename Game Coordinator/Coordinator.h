#pragma once

#include "GC_pch.h"

class Coordinator {
public:
	bool init(); // Initalize winsock
	bool startServer(); // Prepare game coordinator
	bool connectionsThread(); // Thread for accepting client connections
	bool clientThread(SOCKET clientSocket); // Thread for handling an individual client
private:
	const char* PORT_NUM = "56535";
	const char* HOSTNAME = "mae_ahcompsci_gc";

	struct addrinfo* m_result = NULL, * ptr = NULL, hints;

	std::thread*				m_connectionsThread;
	std::vector<std::thread>	m_clientThreads;
};
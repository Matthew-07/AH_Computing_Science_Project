#include "Coordinator.h"

bool Coordinator::run()
{
	m_userConnectionsThread = new std::thread([&]() {userConnectionsThread(); });
	m_serverConnectionsThread = new std::thread([&]() {gameServerConnectionsThread(); });

	m_userConnectionsThread->join();
	m_serverConnectionsThread->join();
	return true;
}

bool Coordinator::init()
{
	WSADATA wsaData;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return false;
	}
	return true;
}

bool Coordinator::startServer()
{
	struct addrinfo* result = NULL, * ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	int iResult = getaddrinfo(NULL, USER_PORT, &hints, &result);
	if (iResult != 0) {
		printf("client getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return false;
	}

	m_UserListenSocket = INVALID_SOCKET;

	// Create a SOCKET for the server to listen for client connections

	m_UserListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (m_UserListenSocket == INVALID_SOCKET) {
		printf("Error at client socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	// Setup the TCP listening socket
	iResult = bind(m_UserListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("client bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(m_UserListenSocket);
		WSACleanup();
		return false;
	}

	freeaddrinfo(result);

	// Game Server Socket

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, GAMESERVER_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return false;
	}

	m_GameServerListenSocket = INVALID_SOCKET;

	// Create a SOCKET for the server to listen for client connections

	m_GameServerListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (m_GameServerListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	// Setup the TCP listening socket
	iResult = bind(m_GameServerListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(m_GameServerListenSocket);
		WSACleanup();
		return false;
	}

	freeaddrinfo(result);

	return true;
}

bool Coordinator::userConnectionsThread() {
	while (true) {
		if (listen(m_UserListenSocket, SOMAXCONN) == SOCKET_ERROR) {
			printf("Listen failed with error: %ld\n", WSAGetLastError());
			closesocket(m_UserListenSocket);
			WSACleanup();
			return false;
		}

		SOCKET ClientSocket = INVALID_SOCKET;

		// Accept a client socket
		ClientSocket = accept(m_UserListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed: %d\n", WSAGetLastError());
		}

		m_userThreads.push_back(std::thread([&]() {gameServerThread((LPVOID)ClientSocket); }));
	}

	return true;
}

bool Coordinator::gameServerConnectionsThread() {
	SOCKET ClientSocket;

	while (true) {
		if (listen(m_GameServerListenSocket, SOMAXCONN) == SOCKET_ERROR) {
			printf("Listen failed with error: %ld\n", WSAGetLastError());
			closesocket(m_GameServerListenSocket);
			WSACleanup();
			return false;
		}

		// Accept a client socket
		ClientSocket = accept(m_GameServerListenSocket, NULL, NULL);
		//if (ClientSocket == INVALID_SOCKET) {
		//	printf("accept failed: %d\n", WSAGetLastError());
		//}

		m_serverThreads.push_back(std::thread([&]() {gameServerThread((LPVOID) ClientSocket); }));
	}

	return true;
}

bool Coordinator::userThread(LPVOID lParam)
{
	/*

	1. Recieve login attempts until one is successful
	2. Send profile information
	3. Wait for commands

	*/

	SOCKET userSocket = (SOCKET)lParam;

	return false;
}

bool Coordinator::gameServerThread(LPVOID lParam)
{
	/*

	1. Wait for security key
	2. Poll for information e.g. maximum number of games it will support concurrently
	3. Add to list of available servers

	*/

	SOCKET serverSocket = (SOCKET)lParam;

	// Test
	const int buffLen = 512;
	char recvBuff[buffLen];

	while (true) {
		int msgSize = recv(serverSocket, recvBuff, buffLen, 0);
		if (msgSize > 0) {
			std::string str = recvBuff;
			str.erase(std::find(str.begin(), str.end(), '\3'), str.end());

			std::cout << "Message Recieved: " << str << std::endl << std::endl;
		}
		else if (msgSize == SOCKET_ERROR) {
			std::cout << "Connection lost" << std::endl;
			closesocket(serverSocket);
			return false;
		}
		//else { Sleep(0.01); }
	}

	std::cout << "Thread exit";
	return false;
}
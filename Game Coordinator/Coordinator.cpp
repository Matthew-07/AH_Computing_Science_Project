#include "Coordinator.h"
#include "Database.h"

bool Coordinator::run()
{
	m_userConnectionsThread = new std::thread([&]() {userConnectionsThread(); });
	m_serverConnectionsThread = new std::thread([&]() {gameServerConnectionsThread(); });

	m_userConnectionsThread->join();

	m_serverConnectionsThread->join();
	return true;
}

Coordinator::Coordinator(Database* db)
{
	m_db = db;
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
	hints.ai_family = AF_INET6;
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

	// None of the settings stored by the addrinfo object need to be changed

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
	SOCKET ClientSocket;

	while (true) {
		if (listen(m_UserListenSocket, SOMAXCONN) == SOCKET_ERROR) {
			printf("Listen failed with error: %ld\n", WSAGetLastError());
			closesocket(m_UserListenSocket);
			WSACleanup();
			return false;
		}

		// Accept a client socket
		ClientSocket = accept(m_UserListenSocket, NULL, NULL);
		//if (ClientSocket == INVALID_SOCKET) {
		//	printf("accept failed: %d\n", WSAGetLastError());
		//}

		m_serverThreads.push_back(std::thread([&]() {userThread((LPVOID)ClientSocket); }));
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

	int32_t* userId = new int32_t[1];

	while (true) {
		char recvBuff[66];

		if (!recieveData(userSocket, recvBuff, 66)) {
			// Connection lost
			closesocket(userSocket);
			return false;
		}

		std::string logInAttempt(recvBuff);

		std::string username = logInAttempt.substr(1, 32);
		long long clip = username.find_first_of('#');
		username = username.substr(0, clip);

		std::string password = logInAttempt.substr(33, 32);
		clip = password.find_first_of('#');
		password = password.substr(0, clip);

		// Create Account
		if (logInAttempt[0] == 'Y') {
			*userId = m_db->addUser(username, password);
		}
		// Else log in to existing account
		else {
			*userId = m_db->logIn(username, password);
		}

		//std::string msg("123");
		//char sendBuff[9];
		//strcpy_s(sendBuff, _countof(sendBuff), msg.c_str());
		sendData(userSocket, (char*)userId, sizeof(*userId));

		if (*userId > 0) {
			break;
		}
	}
	
	// send profile information
	// -----------------

	// Wait for commands

	while (true) {
		int32_t* int32Buff = new int32_t; // decide size later
		if (!recieveData(userSocket, (char*) int32Buff, 4))
		{
			// Connection lost
			closesocket(userSocket);
			return false;
		}
		// Respond to command
		switch (*int32Buff) {
		case JOIN_QUEUE:
		{
			*int32Buff = m_serverIPs.size();
			sendData(userSocket, (char*)int32Buff, sizeof(*int32Buff));
			in6_addr* addrBuff = new in6_addr[*int32Buff];
			for (auto ip : m_serverIPs) {
				memcpy(addrBuff, ip, sizeof(in6_addr));
				addrBuff++;
			}
			addrBuff -= *int32Buff;
			sendData(userSocket, (char*)addrBuff, (*int32Buff) * sizeof(in6_addr));

			int64_t* pingBuff = new int64_t[*int32Buff];
			recieveData(userSocket, (char*)pingBuff, (*int32Buff) * sizeof(pingBuff[0]));

			Player p;
			p.id = *userId;
			p.playerSocket = &userSocket;
			std::cout << "Player " << *userId << " requested to join matchmaking queue:\n";
			for (int a = 0; a < *int32Buff; a++) {
				p.addConnection(addrBuff[a],pingBuff[a]);
				for (int i = 0; i < 7; i++) {
					std::cout << addrBuff[a].u.Word[i] << ".";
				}
				std::cout << addrBuff[a].u.Word[7] << "\t";
				std::cout << pingBuff[a] << "\n";
			}

			break;
		}
		case LEAVE_QUEUE:
			break;


		}
	}
	   
	closesocket(userSocket);
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
	const int buffLen = 32;
	char recvBuff[buffLen];

	sockaddr_in6 s;
	int nameSize = sizeof(s);
	getpeername(serverSocket, (sockaddr*) &s, &nameSize);

	m_serverIPs.push_back(&s.sin6_addr);

	while (true) {
		if (recieveData(serverSocket, recvBuff, buffLen)) {
			std::string str = recvBuff;
			//str.erase(std::find(str.begin(), str.end(), '\3'), str.end());

			std::cout << "Message Recieved: " << str << std::endl << std::endl;
		}
		else {
			std::cout << "Connection lost" << std::endl;
			closesocket(serverSocket);

			std::list<in6_addr*>::iterator it;
			for (it = m_serverIPs.begin(); it != m_serverIPs.end(); it++) {
				if (*it == &s.sin6_addr) {
					m_serverIPs.erase(it);
					break;
				}
			}
			return false;
		}
	}
	std::list<in6_addr*>::iterator it;
	for (it = m_serverIPs.begin(); it != m_serverIPs.end(); it++) {
		if (*it == &s.sin6_addr) {
			m_serverIPs.erase(it);
			break;
		}
	}
	return true;
}
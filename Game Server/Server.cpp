#include "Server.h"

bool Server::init() {
	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return false;
	}

	//// Read settings
	//std::string text;
	//std::fstream f("serverconfig.txt",std::ios::in);
	//if (f.is_open()) {
	//	getline(f, text);
	//	m_ip = (text);
	//} else {
	//	return false;
	//}
	return true;
}

bool Server::start()
{
	int iResult;

	// Get ready to accept clients
	struct addrinfo* result = NULL, * ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, CLIENT_PORT_STRING, &hints, &result);
	if (iResult != 0) {
		printf("client getaddrinfo failed: %d\n\n", iResult);
		WSACleanup();
		return false;
	}

	m_UserListenSocket = INVALID_SOCKET;

	// Create a SOCKET for the server to listen for client connections

	m_UserListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (m_UserListenSocket == INVALID_SOCKET) {
		printf("Error at client socket(): %ld\n\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	// Setup the TCP listening socket
	iResult = bind(m_UserListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("client bind failed with error: %d\n\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(m_UserListenSocket);
		WSACleanup();
		return false;
	}

	freeaddrinfo(result);

	m_recievePacketsThread = new std::thread(&Server::recievePackets, this);
	m_userConnectionsThread = new std::thread(&Server::userConnectionsThread, this);


	// Connect to Game Coordinator
	result = NULL;
	ptr = NULL;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	std::ifstream addressFile = std::ifstream("GC_ip.txt");
	char addr[64];
	ZeroMemory(addr,64);
	if (!addressFile) {
		MessageBoxA(NULL, "Failed to load Game Coordinator IP Address.", "Error", NULL);
		return false;
	}
	else {
		addressFile.read(addr, 64);
		addressFile.close();
		//MessageBoxA(NULL, addr, "msg", NULL);
	}

	// Resolve the server address and port
	iResult = getaddrinfo(addr, COORDINATOR_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET CoordinatorSocket = INVALID_SOCKET;

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		CoordinatorSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (CoordinatorSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(CoordinatorSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(CoordinatorSocket);
			CoordinatorSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (CoordinatorSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	fd_set recieveSocket;
	FD_ZERO(&recieveSocket);
	timeval waitTime;
	waitTime.tv_sec = 0;
	waitTime.tv_usec = 100;

	int32_t int32Buff = -1;

	while (true) {
		FD_SET(CoordinatorSocket, &recieveSocket);

		if (select(0, &recieveSocket, NULL, NULL, &waitTime) == SOCKET_ERROR) {
			closesocket(CoordinatorSocket);			
			return false;
		}

		if (FD_ISSET(CoordinatorSocket, &recieveSocket)) {
			if (!recieveData(CoordinatorSocket, (char*) &int32Buff, 4)) {
				std::cout << "Lost connection to game coordinator.";
				closesocket(CoordinatorSocket);
				return false;
			}

			switch (int32Buff) {
			case START_GAME:
				int32_t numberOfPlayers;
				recieveData(CoordinatorSocket, (char*)&numberOfPlayers, sizeof(numberOfPlayers));

				int32_t numberOfTeams;
				recieveData(CoordinatorSocket, (char*)&numberOfTeams, sizeof(numberOfTeams));

				int32_t* playerIds = new int32_t[numberOfPlayers];
				recieveData(CoordinatorSocket, (char*)playerIds, numberOfPlayers * sizeof(playerIds[0]));

				int32_t* playerTeams = new int32_t[numberOfPlayers];
				recieveData(CoordinatorSocket, (char*)playerTeams, numberOfPlayers * sizeof(playerTeams[0]));

				std::cout << "Game data recieved." << std::endl;
				std::cout << numberOfPlayers << std::endl << numberOfTeams << std::endl;
				for (int i = 0; i < numberOfPlayers; i++) {
					std::cout << "Player: " << playerIds[i] << ", Team: " << playerTeams[i] << std::endl;
				}

				Logic logic = Logic(numberOfPlayers, numberOfTeams, playerIds, playerTeams);

				Game newGame;
				newGame.logic = &logic;
				newGame.players = std::list<ConnectedPlayer>();

				int32_t index = 0;

				while (true) {
					for (auto g : m_games) {
						if (g->logic->index == index) {
							index++;
							break;
						}
					}
					break;
				}

				newGame.logic->index = index;

				m_gameThreads.push_back(std::thread(&Server::gameThread, this, &newGame));
				m_games.push_back(&newGame);
				break;
			}
		}
	}

	std::cout << "Connection Lost.";

	return true;
}

bool Server::recievePackets()
{
	SOCKET s;
	struct sockaddr_in6 server, si_other;

	int slen, recv_len;	

	slen = sizeof(si_other);

	//Create a socket
	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf(" Could not create socket : % d ", WSAGetLastError());
	}
	printf(" Socket created.\n ");

	//Prepare the sockaddr_in structure
	memset(&server, 0, sizeof(server));
	server.sin6_family = AF_INET6;
	server.sin6_addr = in6addr_any;
	server.sin6_port = htons(CLIENT_PORT);

	//Bind
	if (bind(s, (struct sockaddr*) &server, sizeof(server)) == SOCKET_ERROR)
	{
		printf(" Bind failed with error code : % d ", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	char buf[BUFFER_SIZE];

	int counter = 0;
	while (true)
	{
		memset(buf, 0, BUFFER_SIZE);

		if ((recv_len = recvfrom(s, buf, BUFFER_SIZE, 0, (struct sockaddr*) &si_other, &slen)) == SOCKET_ERROR)
		{
			printf(" recvfrom() failed with error code : % d ", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		//print details of the client/peer and the data received
		if (counter == 50) {
			char* buff = new char[32];
			ZeroMemory(buff, 32);
			inet_ntop(AF_INET6, &si_other.sin6_addr, buff, 32);
			printf(" Received packet from % s: % d\n ", buff, ntohs(si_other.sin6_port));
			printf(" Data: % i\n ", *((int64_t*) buf));

			counter = 0;
		}

		//now reply the client with the same data
		if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
		{
			printf(" sendto() failed with error code : % d ", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		counter++;
	}

	closesocket(s);
	WSACleanup();

	return 0;
	return false;
}

bool Server::gameThread(Game* game)
{
	std::list<Input> playerInputs;

	SOCKET udpSocket;
	struct sockaddr_in6 server, si_other;

	int slen, recv_len;

	slen = sizeof(si_other);

	//Create a socket
	if ((udpSocket = socket(AF_INET6, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf(" Could not create socket : % d ", WSAGetLastError());
		getchar();
		exit(EXIT_FAILURE);
	}
	printf(" Socket created.\n ");

	//Prepare the sockaddr_in structure
	memset(&server, 0, sizeof(server));
	server.sin6_family = AF_INET6;
	server.sin6_addr = in6addr_any;
	server.sin6_port = htons(GAME_PORT + game->logic->index);

	//Bind
	if (bind(udpSocket, (struct sockaddr*) & server, sizeof(server)) == SOCKET_ERROR)
	{
		printf(" Bind failed with error code : % d ", WSAGetLastError());
		getchar();
		exit(EXIT_FAILURE);
	}

	int buffSize = game->logic->getMaxGamestateSize();
	char* buffer = new char[buffSize];

	game->mutex.lock();
	while (game->players.size() < game->logic->getNumberOfPlayers()) {
		game->mutex.unlock();
		Sleep(10);
		game->mutex.lock();
	}
	game->mutex.unlock();

	Sleep(1000);

	fd_set inputSockets;
	FD_ZERO(&inputSockets);
	timeval waitTime;
	waitTime.tv_sec = 0;
	waitTime.tv_usec = 5;

	auto lastTick = std::chrono::steady_clock::now();

	auto timePerTick = std::chrono::microseconds(1000000 / TICKS_PER_SECOND);

	while (true) {	
		game->mutex.lock();
		for (auto p : game->players) {
			FD_SET(p.socket, &inputSockets);
		}

		if (select(0, &inputSockets, NULL, NULL, &waitTime) == SOCKET_ERROR) {
			int res = WSAGetLastError();
			game->mutex.unlock();
			std::cout << "Failed to check for player input with error: " << res << "." << std::endl;
			//return false;
			continue;
		}

		for (auto p : game->players) {			
			if (FD_ISSET(p.socket, &inputSockets)) {
				Input input;
				if (!recieveData(p.socket, (char*)&input, sizeof(input))) {
					game->mutex.unlock();
					std::cout << "Failed to recieve player input." << std::endl;
					return false;
				}
				playerInputs.emplace_back(input);
			}
		}

		if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - lastTick) >= timePerTick) {
			int32_t result = game->logic->tick(playerInputs);
			if (result >= 0) {
				// The game finished, team (result) won.
			}
			else if (result == -2) {
				// The round ended but the game did not, wait a short while before the next game starts.
				Sleep(4000);
			}
			else {
				ZeroMemory(buffer, buffSize);
				int gameStateSize = game->logic->getGamestate(buffer);

				for (auto p : game->players) {
					if (sendto(udpSocket, buffer, gameStateSize, 0, (sockaddr*)&p.address, sizeof(p.address)) == SOCKET_ERROR) {
						int err = WSAGetLastError();
						std::cout << "Failed to send packet with error: " << err << std::endl;
						std::cout << "Buffer size: " << gameStateSize << "." << std::endl;
					}
				}

				lastTick = std::chrono::steady_clock::now();
			}
		}
		game->mutex.unlock();
	}
	return false;
}

bool Server::userConnectionsThread()
{
	SOCKET ClientSocket;

	while (true) {
		if (listen(m_UserListenSocket, SOMAXCONN) == SOCKET_ERROR) {
			printf("Listen failed with error: %ld\n\n", WSAGetLastError());
			closesocket(m_UserListenSocket);
			WSACleanup();
			return false;
		}

		// Accept a client socket
		ClientSocket = accept(m_UserListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed: %d\n\n", WSAGetLastError());
		}

		m_userThreads.push_back(std::thread(&Server::userThread, this, (LPVOID)ClientSocket));
	}

	return true;
}

bool Server::userThread(LPVOID clientSocket)
{
	SOCKET userSocket = (SOCKET)clientSocket;

	// First recieve userId;

	int32_t userId = -1;
	if (!recieveData(userSocket, (char*)&userId, 4)) {
		closesocket(userSocket);
		return false;
	}

	std::cout << "Player " << userId << " connected." << std::endl << std::endl;

	int32_t udpPort = -1;
	recieveData(userSocket, (char*)&udpPort, 4);

	bool locatingGame = true;

	while (locatingGame) {
		Sleep(1);
		for (auto game : m_games) {
			if (game->logic->checkForUser(userId)) {

				int32_t numberOfPlayers = game->logic->getNumberOfPlayers();
				int32_t numberOfTeams = game->logic->getNumberOfTeams();
				int32_t* playerIds = game->logic->getPlayerIds();
				int32_t* playerTeams = game->logic->getPlayerTeams();
				int32_t maxGamestateSize = 0;
				maxGamestateSize = game->logic->getMaxGamestateSize();

				if (!sendData(userSocket, (char*)&numberOfPlayers, 4)) {
					std::cout << "Failed to send number of players." << std::endl;
					closesocket(userSocket);
					return false;
				}
				if (!sendData(userSocket, (char*)&numberOfTeams, 4)) {
					std::cout << "Failed to send number of teams." << std::endl;
					closesocket(userSocket);
					return false;
				}
				if (!sendData(userSocket, (char*)playerIds, numberOfPlayers * 4)) {
					std::cout << "Failed to send player Ids." << std::endl;
					closesocket(userSocket);
					return false;
				}
				if (!sendData(userSocket, (char*)playerTeams, numberOfPlayers * 4)) {
					std::cout << "Failed to send player Teams." << std::endl;
					closesocket(userSocket);
					return false;
				}
				if (!sendData(userSocket, (char*)&maxGamestateSize, 4)) {
					int res = WSAGetLastError();					
					std::cout << "Failed to send maximum gamestate size." << std::endl;
					std::cout << "Error: " << res << std::endl;
					closesocket(userSocket);
					return false;
				}

				ConnectedPlayer p;

				p.socket = userSocket;
				p.userId = userId;
				sockaddr_in6 addr;
				int addrlen = sizeof(addr);
				getpeername(userSocket, (sockaddr*) &addr, &addrlen);
				addr.sin6_port = htons(udpPort);
				p.address = addr;
				game->mutex.lock();
				game->players.emplace_back(p);
				game->mutex.unlock();

				locatingGame = false;
				break;
			}
		}		
	}	

	return true;
}

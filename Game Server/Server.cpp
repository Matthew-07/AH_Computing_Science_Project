#include "Server.h"

bool Server::init() {
	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return false;
	}
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

	// Start thread to recieve packets and respond to packets for measuring ping
	m_recievePacketsThread = new std::thread(&Server::recievePackets, this);

	// Start thread to accecpt connections from users
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
				if (!recieveData(CoordinatorSocket, (char*)&numberOfPlayers, sizeof(numberOfPlayers))) {
					int res = WSAGetLastError();
					std::cout << "Failed to retrieve numberOfPlayers with error: " << res << ".\n";
					return false;
				}

				int32_t numberOfTeams;
				if (!recieveData(CoordinatorSocket, (char*)&numberOfTeams, sizeof(numberOfTeams))) {
					int res = WSAGetLastError();
					std::cout << "Failed to retrieve numberOfTeams with error: " << res << ".\n";
					return false;
				}

				int32_t* playerIds = new int32_t[numberOfPlayers];
				if (!recieveData(CoordinatorSocket, (char*)playerIds, numberOfPlayers * sizeof(playerIds[0]))) {
					int res = WSAGetLastError();
					std::cout << "Failed to retrieve playerIds with error: " << res << ".\n";
					return false;
				}

				int32_t* playerTeams = new int32_t[numberOfPlayers];
				if (!recieveData(CoordinatorSocket, (char*)playerTeams, numberOfPlayers * sizeof(playerTeams[0]))) {
					int res = WSAGetLastError();
					std::cout << "Failed to retrieve playerTeams with error: " << res << ".\n";
					return false;
				}

				std::cout << "Game data recieved." << std::endl;
				std::cout << numberOfPlayers << std::endl << numberOfTeams << std::endl;
				for (int i = 0; i < numberOfPlayers; i++) {
					std::cout << "Player: " << playerIds[i] << ", Team: " << playerTeams[i] << std::endl;
				}				

				Game newGame;
				newGame.logic = new Logic(numberOfPlayers, numberOfTeams, playerIds, playerTeams);
				newGame.players = new std::list<ConnectedPlayer>();

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

				//m_gameThreads.push_back(std::thread(&Server::gameThread, this, &newGame));
				std::thread t = std::thread(&Server::gameThread, this, &newGame);
				t.detach();

				m_games.push_back(&newGame);
				break;
			}
		}

		// Check if any games have finished
		while (m_results.size() > 0){

			std::cout << "Game Finished." << std::endl << std::endl;

			int32_t intBuff = GAME_FINISHED;
			sendData(CoordinatorSocket, (char*)&intBuff, 4);

			// I think this won't really effect the compiled code it just makes this code a bit more readable
			// As I can use 'res' instead of 'm_results.front()'
			GameResult& res = m_results.front();
			// Send data
			// Game Duration
			sendData(CoordinatorSocket, (char*)&res.duration, 4);

			// Number of Participating Teams
			sendData(CoordinatorSocket, (char*)&res.numberOfTeams, 4);

			// Team Scores
			sendData(CoordinatorSocket, (char*)res.teamScores, res.numberOfTeams * 4);

			// Number of participants in each team
			sendData(CoordinatorSocket, (char*)res.numbersOfParticipants, res.numberOfTeams * 4);

			// Participant IDs
			for (int t = 0; t < res.numberOfTeams; t++) {
				sendData(CoordinatorSocket, (char*)res.participantIDs[t], res.numbersOfParticipants[t] * 4);
			}

			delete[] res.teamScores;
			delete[] res.numbersOfParticipants;

			for (int i = 0; i < res.numberOfTeams; i++) {
				delete[] res.participantIDs[i];
			}
			delete[] res.participantIDs;

			m_results.pop_front();
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
			WSACleanup();
			exit(EXIT_FAILURE);
		}

		// Output port of client
		if (counter == 50) {
			char* buff = new char[32];
			ZeroMemory(buff, 32);
			inet_ntop(AF_INET6, &si_other.sin6_addr, buff, 32);
			printf(" Received packet from % s: % d\n ", buff, ntohs(si_other.sin6_port));
			delete[] buff;
			counter = 0;
		}

		// Send same data back to client
		if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
		{
			printf(" sendto() failed with error code : % d ", WSAGetLastError());
			WSACleanup();
			exit(EXIT_FAILURE);
		}

		counter++;
	}

	closesocket(s);
	WSACleanup();

	return false;
}

bool Server::gameThread(Game* game)
{
	std::list<Input> playerInputs = {};

	SOCKET udpSocket;
	struct sockaddr_in6 server;

	int slen;

	slen = sizeof(server);

	//Create a socket
	if ((udpSocket = socket(AF_INET6, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : % d ", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	printf("UDP Socket created.\n ");

	//Prepare the sockaddr_in structure
	memset(&server, 0, sizeof(server));
	server.sin6_family = AF_INET6;
	server.sin6_addr = in6addr_any;
	server.sin6_port = htons(GAME_PORT + (USHORT)game->logic->index);

	//Bind
	if (bind(udpSocket, (struct sockaddr*) & server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : % d ", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	int buffSize = game->logic->getMaxGamestateSize();
	char* buffer = new char[buffSize];

	while (game->players->size() < game->logic->getNumberOfPlayers()) {
		Sleep(10);
	}

	Sleep(500);

	fd_set inputSockets;
	FD_ZERO(&inputSockets);
	timeval waitTime;
	waitTime.tv_sec = 0;
	waitTime.tv_usec = 0;

	auto startTime = std::chrono::steady_clock::now();

	auto lastTick = std::chrono::steady_clock::now();

	auto timePerTick = std::chrono::microseconds(1000000 / TICKS_PER_SECOND);	

	//bool tempBool = true;

	while (true) {	

		//Temp timer
		//auto temp = std::chrono::steady_clock::now();

		for (auto p : *game->players){
			FD_SET(p.socket, &inputSockets);
		}

		if (select(0, &inputSockets, NULL, NULL, &waitTime) == SOCKET_ERROR) {
			int res = WSAGetLastError();
			std::cout << "Failed to check for player input with error: " << res << "." << std::endl;			
			getchar();
			continue;
		}		

		for (auto p : *game->players) {
			if (FD_ISSET(p.socket, &inputSockets)) {
				Input input;
				if (!recieveData(p.socket, (char*)&input, sizeof(input))) {
					std::cout << "Failed to recieve player input." << std::endl;
					return false;
				}
				playerInputs.emplace_back(input);
			}
		}

		if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - lastTick) >= timePerTick) {
			int32_t intResult = game->logic->tick(playerInputs);
			if (intResult >= 0) {
				// The game finished, team (result) won.

				// Notfiy players the game ended
				for (auto p : *game->players) {
					int32_t intBuff = GAME_FINISHED;
					sendData(p.socket, (char*)&intBuff, sizeof(intBuff));
					closesocket(p.socket);
				}

				closesocket(udpSocket);

				GameResult result;
				auto endTime = std::chrono::steady_clock::now();
				result.duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();

				game->logic->endGame(result);
				m_results.emplace_back(result);

				std::list<Game*>::iterator iter = m_games.begin();
				while (iter != m_games.end()) {
					if ((*iter)->logic->index = game->logic->index) {
						delete (*iter)->players, (*iter)->logic;
						iter = m_games.erase(iter);
						break;
					}
					iter++;
				}

				delete[] buffer;				

				return true;
			}
			else if (intResult == -2) {
				// The round ended but the game did not, wait a short while before the next round starts.
				auto timer = std::chrono::steady_clock::now();
				while (true) {
					if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - lastTick)
						>= timePerTick) {
						sendGamestate(udpSocket, game, buffer, buffSize);
						lastTick = std::chrono::steady_clock::now();
					}
					if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - timer).count()
						>= 500000) break;
				}
			}
			else {				
				sendGamestate(udpSocket, game, buffer, buffSize);
				lastTick = std::chrono::steady_clock::now();
			}
		}
		/*if (tempBool) {
			std::cout << "Loop took " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - temp).count() << "ms to execute." << std::endl;
			tempBool = false;
		}*/
	}
	return false;
}

void Server::sendGamestate(SOCKET udpSocket, Game* game, char* buff, int32_t buffSize)
{
	ZeroMemory(buff, buffSize);
	int gameStateSize = game->logic->getGamestate(buff);

	if (gameStateSize > buffSize) {
		std::cout << "Gamestate larger than buffer!" << std::endl;
		return;
	}

	for (auto p : *game->players) {
		if (sendto(udpSocket, buff, gameStateSize, 0, (sockaddr*)&p.address, sizeof(p.address)) == SOCKET_ERROR) {
			int err = WSAGetLastError();
			std::cout << "Failed to send packet with error: " << err << std::endl;
			std::cout << "Buffer size: " << gameStateSize << "." << std::endl;
		}
	}
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
			continue;
		}

		//m_userThreads.push_back(std::thread(&Server::userThread, this, (LPVOID)ClientSocket));
		std::thread t = std::thread(&Server::userThread, this, (LPVOID)ClientSocket);
		t.detach();
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
				ZeroMemory(&addr, sizeof(addr));
				int addrlen = sizeof(addr);

				getpeername(userSocket, (sockaddr*) &addr, &addrlen);
				addr.sin6_port = htons((USHORT) udpPort);
				addr.sin6_family = AF_INET6;

				p.address = addr;

				game->players->push_back(p);

				locatingGame = false;
				break;
			}
		}		
	}

	return true;
}

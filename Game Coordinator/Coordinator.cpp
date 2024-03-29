#include "Coordinator.h"
#include "Database.h"

std::mutex mtx;

bool Coordinator::run()
{
	m_userConnectionsThread = new std::thread(&Coordinator::userConnectionsThread,this);
	m_serverConnectionsThread = new std::thread(&Coordinator::gameServerConnectionsThread, this);

	m_matchmakingThread = new std::thread(&Coordinator::matchmakingThread, this);

	m_userConnectionsThread->join();
	m_serverConnectionsThread->join();

	m_matchmakingThread->join();

	delete m_userConnectionsThread, m_serverConnectionsThread, m_matchmakingThread;

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
		printf("WSAStartup failed: %d\n\n", iResult);
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

	// Game Server Socket

	// None of the settings stored by the addrinfo object need to be changed

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, GAMESERVER_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n\n", iResult);
		WSACleanup();
		return false;
	}

	m_GameServerListenSocket = INVALID_SOCKET;

	// Create a SOCKET for the server to listen for client connections

	m_GameServerListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (m_GameServerListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	// Setup the TCP listening socket
	iResult = bind(m_GameServerListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n\n", WSAGetLastError());
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

		//m_userThreads.push_back(std::thread(&Coordinator::userThread, this, (LPVOID) ClientSocket));
		std::thread t = std::thread(&Coordinator::userThread, this, (LPVOID)ClientSocket);
		t.detach();
	}

	return true;
}

bool Coordinator::gameServerConnectionsThread() {
	SOCKET ClientSocket;	

	while (true) {
		if (listen(m_GameServerListenSocket, SOMAXCONN) == SOCKET_ERROR) {
			printf("Listen failed with error: %ld\n\n", WSAGetLastError());
			closesocket(m_GameServerListenSocket);
			WSACleanup();
			return false;
		}

		// Accept a client socket
		ClientSocket = accept(m_GameServerListenSocket, NULL, NULL);
		//if (ClientSocket == INVALID_SOCKET) {
		//	printf("accept failed: %d\n", WSAGetLastError());
		//}

		//m_serverThreads.push_back(std::thread(&Coordinator::gameServerThread, this, (LPVOID) ClientSocket));
		std::thread t = std::thread(&Coordinator::gameServerThread, this, (LPVOID)ClientSocket);
		t.detach();
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
	int32_t userId;
	std::list<COMMAND> tasks;
	
	// Used by goto. A goto just seemed a cleaner way of going back here if the user changes account.
	USER_THREAD_START:

	userId = -1;
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
			userId = m_db->addUser(username, password);
		}
		// Else log in to existing account
		else {
			userId = m_db->logIn(username, password);

			for (auto p : m_connectedPlayers) {
				if (userId == p.id) {
					userId = -5;
					break;
				}
			}
		}

		//std::string msg("123");
		//char sendBuff[9];
		//strcpy_s(sendBuff, _countof(sendBuff), msg.c_str());
		sendData(userSocket, (char*)&userId, sizeof(userId));


		if (userId > 0) {	
			PlayerThread player;
			player.id = userId;
			player.threadTasks = &tasks;

			m_connectedPlayers.push_back(player);

			std::cout << "Player " << userId << " has connected, ";
			break;
		}
	}

	// send profile information
	int32_t data[2] = { 0,0 };
	m_db->getUserGameInfo(userId, &data[0], &data[1]);
	std::cout << "they have played " << data[0] << " games and won " << data[1] << " of them." << std::endl << std::endl;

	sendData(userSocket, (char*)data, 8);
	// -----------------

	// Wait for commands

	fd_set recieveSocket;
	FD_ZERO(&recieveSocket);
	timeval waitTime;
	waitTime.tv_sec = 0;
	waitTime.tv_usec = 100;	

	while (true) {

		FD_SET(userSocket, &recieveSocket);
		if (select(0, &recieveSocket, NULL, NULL, &waitTime) == SOCKET_ERROR) {
			printf("Player %i disconnected.\n\n", userId);

			mtx.lock();
			std::list<Player*>::iterator it;
			for (it = m_matchmakingQueue.begin(); it != m_matchmakingQueue.end(); it++) {
				if ((*it)->id = userId) {
					m_matchmakingQueue.erase(it);
					break;
				}
			}
			std::list<PlayerThread>::iterator playerIt = m_connectedPlayers.begin();
			while (playerIt != m_connectedPlayers.end()) {
				if (playerIt->id == userId) {
					m_connectedPlayers.erase(playerIt);
					break;
				}
			}
			mtx.unlock();
			closesocket(userSocket);
			return false;
		}
		if (FD_ISSET(userSocket, &recieveSocket)) {
			int32_t int32Buff = 0;
			if (!recieveData(userSocket, (char*)&int32Buff, 4))
			{
				// Connection lost
				printf("Player %i disconnected.\n\n", userId);
				mtx.lock();
				std::list<Player*>::iterator it;
				for (it = m_matchmakingQueue.begin(); it != m_matchmakingQueue.end(); it++) {
					if ((*it)->id = userId) {
						m_matchmakingQueue.erase(it);
						break;
					}
				}
				std::list<PlayerThread>::iterator playerIt = m_connectedPlayers.begin();
				while (playerIt != m_connectedPlayers.end()) {
					if (playerIt->id == userId) {
						m_connectedPlayers.erase(playerIt);
						break;
					}
				}
				mtx.unlock();
				closesocket(userSocket);

				return false;
			}
			// Respond to command
			switch (int32Buff) {
			case JOIN_QUEUE:
			{
				mtx.lock();
				int32Buff = m_servers.size();

				sendData(userSocket, (char*)&int32Buff, sizeof(int32Buff));
				in6_addr* addrBuff = new in6_addr[int32Buff];
				for (auto server : m_servers) {
					memcpy(addrBuff, server->ip, sizeof(in6_addr));
					addrBuff++;
				}
				mtx.unlock();

				addrBuff -= int32Buff;
				sendData(userSocket, (char*)addrBuff, int32Buff * sizeof(in6_addr));

				if (int32Buff == 0) {
					std::cout << "Player " << userId << " requested to join the matchmaking queue but there are no servers available.\n\n";
					continue;
				}

				int64_t* pingBuff = new int64_t[int32Buff];
				recieveData(userSocket, (char*)pingBuff, int32Buff * sizeof(pingBuff[0]));

				bool usableServers = false;
				for (int i = 0; i < int32Buff; i++) {
					if (pingBuff[i] >= 0) {
						usableServers = true;
					}
				}

				if (!usableServers) {
					std::cout << "Player " << userId << " requested to join the matchmaking queue but the player does not have a suitable connection to any of the servers.\n\n";
					continue;
				}

				Player p;
				p.id = userId;
				p.playerSocket = &userSocket;
				p.threadTasks = &tasks;
				std::cout << "Player " << userId << " requested to join the matchmaking queue:" << std::endl;

				// Insertion sort
				for (int i = 1; i < int32Buff; i++) {
					int64_t tempPing = pingBuff[i];
					in6_addr tempIp = addrBuff[i];

					int index = i;
					while (index > 0 and tempPing < pingBuff[index - 1]) {
						pingBuff[index] = pingBuff[index - 1];
						addrBuff[index] = addrBuff[index - 1];
						index--;
					}
					pingBuff[index] = tempPing;
					addrBuff[index] = tempIp;
				}

				for (int a = 0; a < int32Buff; a++) {
					p.addConnection(addrBuff[a], pingBuff[a]);
					char str[64];
					inet_ntop(AF_INET6, addrBuff + a, str, 64);
					std::cout << str << ":\t" << pingBuff[a] << "\n";
				}

				delete[] pingBuff, addrBuff;

				mtx.lock();
				m_matchmakingQueue.push_back(&p);
				mtx.unlock();

				std::cout << std::endl;
				break;
			}
			case LEAVE_QUEUE:
			{				std::cout << "Player " << userId << " requested to leave the matchmaking queue." << std::endl << std::endl;

			bool playerFound = false;
			mtx.lock();
			std::list<Player*>::iterator it;
			for (it = m_matchmakingQueue.begin(); it != m_matchmakingQueue.end(); it++) {
				if ((*it)->id = userId) {
					m_matchmakingQueue.erase(it);
					playerFound = true;
					break;
				}
			}
			mtx.unlock();
			if (playerFound) {
				// Player Successfully left the queue
				int32_t buff = LEFT_QUEUE;
				sendData(userSocket, (char*)&buff, 4);

				printf("Player %i left the queue.\n\n", userId);
			}
			break;
			}
			case SWITCH_ACCOUNT:
			{
				printf("Player %i has logged out.\n\n", userId);
				std::list<PlayerThread>::iterator playerIt = m_connectedPlayers.begin();
				while (playerIt != m_connectedPlayers.end()) {
					if (playerIt->id == userId) {
						m_connectedPlayers.erase(playerIt);
						break;
					}
				}
				goto USER_THREAD_START;
			}
			}
		}

		// Carry out any tasks required by other threads.
		//mtx.lock();
		while (tasks.size() > 0) {
			switch (tasks.front().type) {
			case USER_NEWGAME:
			{
				int32_t buff = GAME_FOUND;
				sendData(userSocket, (char*)&buff, 4);

				sendData(userSocket, (char*)tasks.front().data, sizeof(*(IN6_ADDR*)tasks.front().data));
				tasks.pop_front();
				break;
			}
			case USER_STATS:
			{
				// send profile information
				int32_t data[2] = { 0,0 };
				m_db->getUserGameInfo(userId, &data[0], &data[1]);

				sendData(userSocket, (char*)data, 8);
				tasks.pop_front();
				break;
			}
			}
		}
		//mtx.unlock();
	}

	std::list<PlayerThread>::iterator playerIt = m_connectedPlayers.begin();
	while (playerIt != m_connectedPlayers.end()) {
		if (playerIt->id == userId) {
			m_connectedPlayers.erase(playerIt);
			break;
		}
	}
	closesocket(userSocket);
	return false;
}

bool Coordinator::gameServerThread(LPVOID lParam)
{
	SOCKET serverSocket = (SOCKET)lParam;

	std::list<COMMAND> tasks;

	sockaddr_in6 s;
	int nameSize = sizeof(s);
	getpeername(serverSocket, (sockaddr*)&s, &nameSize);

	char serverAddrBuff[64];
	inet_ntop(AF_INET6, &s.sin6_addr, serverAddrBuff, 64);
	std::cout << "New server connected: " << serverAddrBuff << std::endl << std::endl;

	Server server;
	server.ip = &s.sin6_addr;
	server.socket = &serverSocket;
	server.threadTasks = &tasks;

	mtx.lock();
	m_servers.push_back(&server);
	mtx.unlock();

	fd_set recieveSocket;
	FD_ZERO(&recieveSocket);
	timeval waitTime;
	waitTime.tv_sec = 0;
	waitTime.tv_usec = 100;

	while (true) {

		FD_SET(serverSocket, &recieveSocket);

		if (select(0, &recieveSocket, NULL, NULL, &waitTime) == SOCKET_ERROR) {
			closesocket(serverSocket);
			std::cout << "Connection lost" << std::endl << std::endl;

			mtx.lock();
			std::list<Server*>::iterator it;
			for (it = m_servers.begin(); it != m_servers.end(); it++) {
				if ((*it)->ip == &s.sin6_addr) {
					m_servers.erase(it);
					break;
				}
			}
			mtx.unlock();
			return false;
		}

		if (FD_ISSET(serverSocket, &recieveSocket)) {
			int32_t int32Buff;
			if (!recieveData(serverSocket, (char*)&int32Buff, 4))
			{
				std::cout << "Connection lost" << std::endl << std::endl;
				closesocket(serverSocket);

				mtx.lock();
				std::list<Server*>::iterator it;
				for (it = m_servers.begin(); it != m_servers.end(); it++) {
					if ((*it)->ip == &s.sin6_addr) {
						m_servers.erase(it);
						break;
					}
				}
				mtx.unlock();
				return false;
			}
			switch (int32Buff) {
			case GAME_FINISHED:
			{
				std::cout << "A game has finished successfully." << std::endl << std::endl;

				GameInfo game;
				
				time_t now = time(0);
				tm ltm;
				localtime_s(&ltm, &now);
				// Date is in 'DD/MM/YYYY' format plus a NULL character at the end
				const char* format = "%02d/%02d/%04d";
				sprintf_s(game.date, format, ltm.tm_mday, ltm.tm_mon + 1, ltm.tm_year + 1900);

				// Game Duration
				game.duration = 0;
				recieveData(serverSocket, (char*)&game.duration, 4);

				// Number of Participating Teams
				game.numberOfTeams = 0;
				recieveData(serverSocket, (char*)&game.numberOfTeams, 4);

				// Team Scores
				game.scores = new int32_t[game.numberOfTeams];
				recieveData(serverSocket, (char*)game.scores, game.numberOfTeams * 4);

				// Number of participants in each team
				game.numbersOfParticipants = new int32_t[game.numberOfTeams];
				recieveData(serverSocket, (char*)game.numbersOfParticipants, game.numberOfTeams * 4);

				// Participant IDs
				game.participants = new int32_t*[game.numberOfTeams];
				for (int t = 0; t < game.numberOfTeams; t++) {
					game.participants[t] = new int32_t[game.numbersOfParticipants[t]];
					recieveData(serverSocket, (char*)game.participants[t], game.numbersOfParticipants[t] * 4);
				}

				m_db->addGame(game);

				// Tell user threads to resend user stats
				for (int t = 0; t < game.numberOfTeams; t++) {
					for (int p = 0; p < game.numbersOfParticipants[t]; p++) {
						for (auto connectedPlayer : m_connectedPlayers) {
							if (connectedPlayer.id == game.participants[t][p]) {
								COMMAND c;
								c.type = USER_STATS;
								connectedPlayer.threadTasks->push_back(c);
							}
						}
					}
				}							

				delete[] game.scores;
				for (int t = 0; t < game.numberOfTeams; t++) {
					delete[] game.participants[t];
				}
				delete[] game.participants;
			}
			}
		}
		//mtx.lock();
		while (tasks.size() > 0) {
			switch (tasks.front().type) {			
			case SERVER_NEWGAME:
			{
				int32_t buffer;
				buffer = START_GAME;
				sendData(serverSocket, (char*)&buffer, sizeof(buffer));

				buffer = reinterpret_cast<GAME*>(tasks.front().data)->numberOfPlayers;
				sendData(serverSocket, (char*)&buffer, sizeof(buffer));

				int32_t numberOfTeams = reinterpret_cast<GAME*>(tasks.front().data)->numberOfTeams;
				sendData(serverSocket, (char*)&numberOfTeams, sizeof(int32_t));

				sendData(serverSocket, (char*)(reinterpret_cast<GAME*>(tasks.front().data)->userIds), sizeof(int32_t)* reinterpret_cast<GAME*>(tasks.front().data)->numberOfPlayers);

				sendData(serverSocket, (char*)(reinterpret_cast<GAME*>(tasks.front().data)->teams), sizeof(int32_t)* reinterpret_cast<GAME*>(tasks.front().data)->numberOfPlayers);
				for (int i = 0; i < buffer; i++) {
					std::cout << reinterpret_cast<GAME*>(tasks.front().data)->userIds[i] << '\t';
					std::cout << reinterpret_cast<GAME*>(tasks.front().data)->teams[i] << std::endl;
				}

				tasks.pop_front();
				break;
			}
			}
		}
		//mtx.unlock();
	}
	mtx.lock();
	std::list<Server*>::iterator it;
	for (it = m_servers.begin(); it != m_servers.end(); it++) {
		if ((*it)->ip == &s.sin6_addr) {
			m_servers.erase(it);
			break;
		}
	}
	mtx.unlock();
	return true;
}

bool Coordinator::matchmakingThread()
{
	std::list<Player*>::iterator playerA, playerB;
	int maxPing = 120;
	bool foundGame;
	mtx.lock();
	while (true) {
		
		mtx.unlock();
		Sleep(2);
		mtx.lock();

		/*
		Two players A and B.
		Initally player A is first in the queue and player B is second.
		Whilst keeping player A the same, go through each player in the queue and see if a game can be made between the two.
		If none can be made, the second player in the queue becomes player A and the process repeats.
		*/			
		playerA = m_matchmakingQueue.begin();
		playerB = std::next(m_matchmakingQueue.begin(), 1);
		foundGame = false;
		while (playerA != std::prev(m_matchmakingQueue.end(),1)){
			while (playerB != m_matchmakingQueue.end()){
				// Try to match players in a game.

				// For now, all that is needed a server which they both have a low ping with.
				for (int s1 = 0; s1 < (*playerA)->pings.size(); s1++) {
					if ((*playerA)->pings[s1].ping < 0 || (*playerA)->pings[s1].ping > 120) continue;
					for (int s2 = 0; s2 < (*playerB)->pings.size(); s2++) {							
						bool serverIsSame = true;
						for (int w = 0; w < 8; w++) {
							if ((*playerA)->pings[s1].serverIP.u.Word[w] != (*playerB)->pings[s2].serverIP.u.Word[w]) {
								serverIsSame = false;
								break;
							}
						}							
						if (serverIsSame) {
							if ((*playerB)->pings[s2].ping < 0 || (*playerB)->pings[s2].ping > 120) {
								break;
							}
							else {
								// Create game
								// First request server to host game
								// Find server object
								bool serverFound = false;
								std::list<Server*>::iterator serverIt;
								for (serverIt = m_servers.begin(); serverIt != m_servers.end(); serverIt++ ) {
									bool serverIsSame = true;
									for (int w = 0; w < 8; w++) {
										if ((*playerA)->pings[s1].serverIP.u.Word[w] != (*serverIt)->ip->u.Word[w]) {
											serverIsSame = false;
											break;
										}
									}
									if (serverIsSame) {
										serverFound = true;
										break;
									}										
								}

								if (!serverFound) {
									// Server must have been disconnected, don't try to use it again.
									(*playerA)->pings[s1].ping = -1;
									(*playerB)->pings[s2].ping = -1;
									continue;
								}

								// Notify server about the game
								COMMAND c;

								GAME game;
								game.numberOfPlayers = 2;
								game.numberOfTeams = 2;
								game.userIds = new int32_t[2];
								game.userIds[0] = (*playerA)->id;
								game.userIds[1] = (*playerB)->id;
								game.teams = new int32_t[2];
								game.teams[0] = 0;
								game.teams[1] = 1;
								c.type = SERVER_NEWGAME;
								c.data = &game;

								(*serverIt)->threadTasks->push_back(c);

								// Notify players they have been found a game.									
								c.type = USER_NEWGAME;
								c.data = (*serverIt)->ip;
								(*playerA)->threadTasks->push_back(c);
								(*playerB)->threadTasks->push_back(c);

								char ipBuff[64];
								inet_ntop(AF_INET6, (*serverIt)->ip, ipBuff, 64);
								printf("A game was created between players %i and %i\non server %s.\n", (*playerA)->id, (*playerB)->id, ipBuff);

								// Remove players from matchmaking queue
								m_matchmakingQueue.erase(playerA++);
								m_matchmakingQueue.erase(playerB++);
								foundGame = true;
							}		
						}
						if (foundGame) break;
					}
					if (foundGame) break;
				}
				if (foundGame) break;
				++playerB;
			}
			if (foundGame) break;
			++playerA;
		}		
	}
	return false;
}

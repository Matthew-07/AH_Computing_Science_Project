#include "Network.h"

bool Network::init() {
	WSADATA wsaData;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		MessageBoxA(NULL, "Failed to connect to server.", "Error", NULL);
		return false;
	}

	// Initalise UDP Socket
	if ((m_udpSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) {
		MessageBoxA(NULL, "Failed to create UDP Socket.", "Error", NULL);
		return false;
	}

	// Set timeout time for recv on UDP socket. It will never wait more than 500ms for a packet.
	DWORD timeout = 500;
	if (setsockopt(m_udpSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
		MessageBoxA(NULL, "Failed to set socket option:\nUDP Timeout", "Error", NULL);
		return false;
	}

	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo("DESKTOP-BJ9V93R", COORDINATOR_PORT, &hints, &result);
	if (iResult != 0) {
		std::string err("getaddrinfo failed with error: %d\n");
		char buff[64];
		sprintf_s(buff,_countof(buff),err.c_str(), iResult);
		MessageBoxA(NULL, buff, "Error", NULL);
		WSACleanup();
		return false;
	}

	m_GCSocket = INVALID_SOCKET;

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		m_GCSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (m_GCSocket == INVALID_SOCKET) {
			//printf("socket failed with error: %ld\n", WSAGetLastError());
			MessageBoxA(NULL, "Failed to connect to server.", "Error", NULL);
			WSACleanup();
			return false;
		}

		// Connect to server.
		iResult = connect(m_GCSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(m_GCSocket);
			m_GCSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (m_GCSocket == INVALID_SOCKET) {
		MessageBoxA(NULL, "Failed to connect to server.", "Error", NULL);;
		WSACleanup();
		return false;
	}

	return true;
}

int Network::logIn(bool newAccount, std::string username, std::string password)
{
	char buffer[66];
	strcpy_s(buffer, _countof(buffer), (newAccount) ? "Y" : "N");

	int usernameLength = username.length();
	username.resize(32);
	std::fill(username.begin() + usernameLength, username.end(), L'#');
	strcat_s(buffer, _countof(buffer), username.c_str());

	int passwordLength = password.length();
	password.resize(32);
	std::fill(password.begin() + passwordLength, password.end(), L'#');
	strcat_s(buffer, _countof(buffer), password.c_str());

	OutputDebugStringA(buffer);
	OutputDebugStringA("\n");

	sendData(m_GCSocket, buffer, 66);

	int32_t * returnBuffer = new int();
	*returnBuffer = -1000;
	//strcpy_s(returnBuffer, _countof(returnBuffer), "###");
	recieveData(m_GCSocket, (char*)returnBuffer, sizeof(*returnBuffer));

	OutputDebugStringA(std::to_string(*returnBuffer).c_str());
	OutputDebugStringA("\n");

	int id = *returnBuffer;
	delete returnBuffer;
		
	return id;
}

bool Network::joinMatchmakingQueue()
{
	int32_t* buffer = new int32_t;
	*buffer = JOIN_QUEUE;
	if (!sendData(m_GCSocket, (char*) buffer, sizeof(*buffer))){
		MessageBoxA(NULL, "Failed to send join queue request.", "Error", NULL);
		return false;
	}

	*buffer = -1;
	if (!recieveData(m_GCSocket, (char*)buffer, sizeof(*buffer))) {
		MessageBoxA(NULL, "Failed to recieve server count.", "Error", NULL);
		return false;
	}
	if (*buffer <= 0) { 
		MessageBoxA(NULL, "No servers available.", "Error", NULL);
		return false;
	}

	in6_addr* serverListBuffer = new in6_addr[(*buffer)];
	if (!recieveData(m_GCSocket, (char*)serverListBuffer, *buffer * sizeof(*serverListBuffer))) {
		MessageBoxA(NULL, "Failed to recieve server IPs.", "Error", NULL);
		return false;
	}

	int64_t* pingBuff = new int64_t[*buffer];

	checkPings(serverListBuffer, pingBuff, *buffer);

	if (!sendData(m_GCSocket, (char*)pingBuff, *buffer * sizeof(pingBuff[0]))) {
		MessageBoxA(NULL, "Failed to send pings.", "Error", NULL);
		return false;
	}

	delete[] pingBuff, serverListBuffer;
	delete buffer;

	return true;
}

void Network::checkPings(in6_addr* addressBuffer, int64_t* avgPingBuffer, const int numberOfServers)
{
	if (numberOfServers <= 0) {
		MessageBoxA(NULL, "No servers available.", "Error", NULL);
		return;
	}

	std::thread* sendThreads = new std::thread[numberOfServers];
	std::thread* recieveThreads = new std::thread[numberOfServers];

	int64_t** pingBuff = new int64_t * [numberOfServers];

	// First check there are no packets already queued as these would cause an incorrect result.
	char* checkBuff = new char[8];
	fd_set checkSocket;
	FD_ZERO(&checkSocket);
	timeval waitTime;
	waitTime.tv_sec = 0;
	waitTime.tv_usec = 1000; // Wait 1ms only
	while (true) {
		FD_SET(m_udpSocket, &checkSocket);
		if (select(0, &checkSocket, NULL, NULL, &waitTime) == SOCKET_ERROR) {
			return;
		}
		if (FD_ISSET(m_udpSocket, &checkSocket)) {
			recvfrom(m_udpSocket, checkBuff, 8, 0, NULL, NULL);
		}
		else {
			break;
		}
	}
	delete checkBuff;

	for (int s = 0; s < numberOfServers; s++) {

		
		

		pingBuff[s] = new int64_t[100];

		for (int p = 0; p < 100; p++) {
			pingBuff[s][p] = -1;
		}

		avgPingBuffer[s] = 0;

		sockaddr_in6 addr;
		int slen = sizeof(addr);
		memset(&addr, 0, slen);
		addr.sin6_family = AF_INET6;
		addr.sin6_addr = addressBuffer[s];
		addr.sin6_port = htons(26533);

		sendThreads[s] = std::thread(&Network::sendPings, this, (sockaddr*) &addr, slen);
		recieveThreads[s] = std::thread(&Network::recievePings, this, (sockaddr*) &addr, pingBuff[s], slen);
	}

	for (int s = 0; s < numberOfServers; s++) {
		sendThreads[s].join();
		recieveThreads[s].join();

		int successfulPings = 0;
		for (int p = 0; p < 100; p++) {
			if (pingBuff[s][p] >= 0) {
				avgPingBuffer[s] += pingBuff[s][p];
				successfulPings++;
			}
			else {
				break;
			}
		}
		// No more than 5% packetloss
		if (successfulPings >= 95) {
			avgPingBuffer[s] /= successfulPings;
		}
		else {
			// Tells GC not to use this server.
			avgPingBuffer[s] = -1;
		}
	}

	delete[] pingBuff, sendThreads, recieveThreads;
}

void Network::sendPings(sockaddr* address, int slen)
{
	int64_t* buff = new int64_t;
	for (int i = 0; i < 100; i++) {
		*buff = (std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()
			)).count();	
//		Sleep(2);
		if (sendto(m_udpSocket, (char*)buff, sizeof(*buff), 0, address, slen) == SOCKET_ERROR) {
			char buff[32];
			sprintf_s(buff,"%i",WSAGetLastError());
			OutputDebugStringA("Error: ");
			OutputDebugStringA(buff);
			OutputDebugStringA("\n");
			exit(EXIT_FAILURE);
		}
	}
	delete buff;
}

void Network::recievePings(sockaddr* address, int64_t* pingBuffer, int slen)
{
	int64_t* buff = new int64_t;
	for (int i = 0; i < 100; i++) {
		if (recvfrom(m_udpSocket, (char*)buff, sizeof(*buff), 0, address, &slen) == SOCKET_ERROR) {
			break;
		}
		else {
			pingBuffer[i] = (std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()
				)).count() - (*buff);
		}
	}
	delete buff;
}


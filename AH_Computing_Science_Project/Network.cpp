#include "Network.h"

bool Network::init() {
	WSADATA wsaData;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		MessageBoxA(NULL, "Failed to connect to server.", "Error", NULL);
		return false;
	}	

	startConnection();

	m_udpSocket = INVALID_SOCKET;

	// Initalise UDP Socket
	if ((m_udpSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) {
		MessageBoxA(NULL, "Failed to create UDP Socket.", "Error", NULL);
		return false;
	}

	sockaddr_in6 udpAddress;
	ZeroMemory(&udpAddress, sizeof(udpAddress));
	udpAddress.sin6_family = PF_INET6;
	udpAddress.sin6_addr = in6addr_any;
	udpAddress.sin6_port = htons(26532);

	if (bind(m_udpSocket, (sockaddr*)&udpAddress, sizeof(udpAddress)) == SOCKET_ERROR) {
		char buff[32];
		sprintf_s(buff, "%i", WSAGetLastError());
		OutputDebugStringA("Error: ");
		OutputDebugStringA(buff);
		OutputDebugStringA("\n");
		MessageBoxA(NULL, "Failed to bind UDP Socket.", "Error", NULL);
		return false;
	}

	return true;
}

bool Network::startConnection()
{
	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	std::ifstream addressFile = std::ifstream("GC_ip.txt");
	char addr[64];
	ZeroMemory(addr, 64);
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
	int iResult = getaddrinfo(addr, COORDINATOR_PORT, &hints, &result);
	//iResult = getaddrinfo("2a00:23c4:3149:e300:dcdb:f1c4:c76c:24f2", COORDINATOR_PORT, &hints, &result);
	if (iResult != 0) {
		std::string err("getaddrinfo failed with error: %d\n");
		char buff[64];
		sprintf_s(buff, _countof(buff), err.c_str(), iResult);
		MessageBoxA(NULL, buff, "Error", NULL);
		WSACleanup();
		return false;
	}
	char buff[64];
	inet_ntop(AF_INET6, result->ai_addr, buff, 64);
	//MessageBoxA(NULL, buff, "Server Ip", NULL);

	m_GCSocket = INVALID_SOCKET;

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		m_GCSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (m_GCSocket == INVALID_SOCKET) {
			char buff[512];
			sprintf_s(buff, "Failed to connect to server with error: %i", WSAGetLastError());
			MessageBoxA(NULL, buff, "Error", NULL);
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
		char buff[512];
		sprintf_s(buff, "Failed to connect to server with error: %i", WSAGetLastError());
		MessageBoxA(NULL, buff, "Error", NULL);
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

	bool validServer = false;
	for (int s = 0; s < *buffer; s++) {
		if (pingBuff[s] >= 0) {
			validServer = true;
		}
	}

	if (!validServer) {
		MessageBoxA(NULL, "Connection to servers too poor.", "Error", NULL);
		return false;
	}

	delete[] pingBuff, serverListBuffer;
	delete buffer;

	return true;
}

bool Network::leaveMatchmakingQueue()
{
	int32_t* buffer = new int32_t;
	*buffer = LEAVE_QUEUE;
	if (!sendData(m_GCSocket, (char*)buffer, sizeof(*buffer))) {
		MessageBoxA(NULL, "Failed to send leave queue request.", "Error", NULL);
		return false;
	}

	if (!recieveData(m_GCSocket, (char*)buffer, sizeof(*buffer))) {
		MessageBoxA(NULL, "Failed to recieve leave queue responce.", "Error", NULL);
		return false;
	}

	if (*buffer != 0) {
		// A match has already been found
		return false;
	}

	return true;
}

bool Network::joinGame(in6_addr* serverAddress)
{
	// Setup TCP connection to server
	SOCKET tcpServerSocket = INVALID_SOCKET;

	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	char addrStr[64];
	ZeroMemory(addrStr, 64);
	inet_pton(AF_INET6, addrStr, serverAddress);
	int iResult = getaddrinfo(addrStr, COORDINATOR_PORT, &hints, &result);
	//iResult = getaddrinfo("2a00:23c4:3149:e300:dcdb:f1c4:c76c:24f2", COORDINATOR_PORT, &hints, &result);
	if (iResult != 0) {
		std::string err("getaddrinfo failed with error: %d\n");
		char buff[64];
		sprintf_s(buff, _countof(buff), err.c_str(), iResult);
		MessageBoxA(NULL, buff, "Error", NULL);
		WSACleanup();
		return false;
	}
	char buff[64];
	inet_ntop(AF_INET6, result->ai_addr, buff, 64);
	MessageBoxA(NULL, buff, "Server Ip", NULL);

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		tcpServerSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (tcpServerSocket == INVALID_SOCKET) {
			char buff[512];
			sprintf_s(buff, "Failed to connect to server with error: %i", WSAGetLastError());
			MessageBoxA(NULL, buff, "Error", NULL);
			WSACleanup();
			return false;
		}

		// Connect to server.
		iResult = connect(tcpServerSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(tcpServerSocket);
			tcpServerSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (tcpServerSocket == INVALID_SOCKET) {
		char buff[512];
		sprintf_s(buff, "Failed to connect to server with error: %i", WSAGetLastError());
		MessageBoxA(NULL, buff, "Error", NULL);
		WSACleanup();
		return false;
	}

	// Create UDP socket for recieving datagrams.
	SOCKET udpServerSocket = INVALID_SOCKET;




}

void Network::checkPings(in6_addr* addressBuffer, int64_t* avgPingBuffer, const int numberOfServers)
{
	//if (numberOfServers <= 0) {
	//	MessageBoxA(NULL, "No servers available.", "Error", NULL);
	//	return;
	//}

	// Set timeout time for recieve operations on UDP socket. It will never wait more than 500ms for a packet.
	DWORD timeout = 500;
	if (setsockopt(m_udpSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
		MessageBoxA(NULL, "Failed to set socket option:\nUDP Timeout", "Error", NULL);
		return;
	}	

	int64_t** pingBuff = new int64_t * [numberOfServers];

	sockaddr_in6* addr = new sockaddr_in6[numberOfServers];

	for (int s = 0; s < numberOfServers; s++) {

		pingBuff[s] = new int64_t[100];

		for (int p = 0; p < 100; p++) {
			pingBuff[s][p] = -1;
		}

		avgPingBuffer[s] = 0;
	}

	std::thread* sendThreads = new std::thread[numberOfServers];
	std::thread recieveThread = std::thread(&Network::recievePings, this, m_udpSocket, pingBuff, addressBuffer, numberOfServers);

	for (int s = 0; s < numberOfServers; s++){
		addr[s] = sockaddr_in6();
		int slen = sizeof(addr[s]);
		ZeroMemory(addr+s, slen);
		addr[s].sin6_family = AF_INET6;
		addr[s].sin6_addr = addressBuffer[s];
		addr[s].sin6_port = htons(26533);

		sendThreads[s] = std::thread(&Network::sendPings, this, m_udpSocket, addr+s);		
	}

	recieveThread.join();

	for (int s = 0; s < numberOfServers; s++) {
		sendThreads[s].join();		

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
	//closesocket(udpSocket);

	delete[] pingBuff, sendThreads, addr;
}

void Network::sendPings(SOCKET s, sockaddr_in6* addr)
{
	int64_t* buff = new int64_t;
	for (int i = 0; i < 100; i++) {
		*buff = (std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()
			)).count();	
//		Sleep(2);
		if (sendto(s, (char*)buff, sizeof(*buff), 0, (sockaddr*) addr, sizeof(*addr)) == SOCKET_ERROR) {
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

void Network::recievePings(SOCKET s, int64_t** pingBuffer, in6_addr* addressBuffer, int numberOfServers)
{
	struct sockaddr_in6 inAddress;

	int slen = sizeof(inAddress);

	int32_t* numberOfPackets = new int32_t[numberOfServers];
	for (int s = 0; s < numberOfServers; s++) {
		numberOfPackets[s] = 0;
	}

	int64_t* buff = new int64_t;
	while (true) {
		ZeroMemory(&inAddress, sizeof(inAddress));
		if (recvfrom(s, (char*)buff, sizeof(*buff), 0, (struct sockaddr*) (&inAddress), &slen) == SOCKET_ERROR) {			
			char buff[32];
			sprintf_s(buff, "%i", WSAGetLastError());
			OutputDebugStringA("Error: ");
			OutputDebugStringA(buff);
			OutputDebugStringA("\n");
			//MessageBoxA(NULL, "Recieve Failed.", "Error", NULL);
			break;
		}
		else {
			for (int s = 0; s < numberOfServers; s++) {
				bool isEqual = true;
				for (int w = 0; w < 8; w++) {
					if (inAddress.sin6_addr.u.Word[w] != addressBuffer[s].u.Word[w]) {
						isEqual = false;
						break;
					}
				}
				if (isEqual) {
					pingBuffer[s][numberOfPackets[s]] = (std::chrono::duration_cast<std::chrono::milliseconds>(
						std::chrono::system_clock::now().time_since_epoch()
						)).count() - (*buff);
					numberOfPackets[s]++;
					break;
				}
			}
			
		}
		slen = sizeof(inAddress);
	}	
	delete buff;
}


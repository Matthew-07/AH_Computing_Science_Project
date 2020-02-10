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

	int32_t * returnBuffer = new int[1];
	returnBuffer[0] = -1000;
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
		return false;
	}

	*buffer = -1;
	if (!recieveData(m_GCSocket, (char*)buffer, sizeof(*buffer))) {
		return false;
	}
	if (*buffer <= 0) { return false; }

	in6_addr* serverListBuffer = new in6_addr[(*buffer)];
	if (!recieveData(m_GCSocket, (char*)serverListBuffer, *buffer * sizeof(*serverListBuffer))) {
		return false;
	}

	// TEMP
	int64_t* pingBuff = new int64_t[*buffer];
	

	for (int i = 0; i < *buffer; i++) {
		pingBuff[i] = i;
		sockaddr_in6 address;		
		memset(&address, 0, sizeof(address));
		address.sin6_family = AF_INET6;
		address.sin6_addr = serverListBuffer[i];
		address.sin6_port = htons(26533);
		int slen = sizeof(address);
		int bytesSent;
		if ((bytesSent = sendto(m_udpSocket, (char*)buffer, sizeof(*buffer), 0,(sockaddr*) &address, slen)) == SOCKET_ERROR) {
			char buff[128];
			sprintf_s(buff,"Send failed with error: %ld\n", WSAGetLastError());
			OutputDebugStringA(buff);
		}
		char b[32];
		_itoa_s(bytesSent, b, 10);
		OutputDebugStringA("\nBytes: ");
		OutputDebugStringA(b);
		OutputDebugStringA("\n");
	}
	if (!sendData(m_GCSocket, (char*)pingBuff, *buffer * sizeof(pingBuff[0]))) {
		return false;
	}
	// ====



	//for (int i = 0; i < (*buffer); i++) {

	//	/*
	//	Start thread which does the following:

	//	1. Send packets to server containing the current time

	//	and another which

	//	2. Wait for responses, if none is recieved in 500ms, the connection is too slow, use -1.

	//	3. If a responce is recieved, compare the time in the packet to what is now the current time, return this value.

	//	*/

	//	for (int p = 0; p < 100; p++) {
	//		int64_t* buff = new int64_t;
	//		*buff = (std::chrono::duration_cast<std::chrono::milliseconds>(
	//			std::chrono::system_clock::now().time_since_epoch()
	//			)).count();			
	//		//sendto(m_udpSocket, (char*) buff, sizeof(*buff), 0, );
	//	}
	//}

	//for (int i = 0; i < (*buffer); i++) {
	//	/*
	//	
	//	Join the threads used in the above for loop back to this thread.
	//	store the return values in a buffer and send it to the game coordinator.

	//	*/
	//	int64_t* buff = new int64_t;
	//	//recvfrom(m_udpSocket, (char*) buff, sizeof(*buff), 0, )
	//}

	// Wait for the game coordinator to respond with details about the game when it is found
}


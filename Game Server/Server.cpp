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

	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;

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
		MessageBoxA(NULL, addr, "msg", NULL);
	}

	// Resolve the server address and port
	iResult = getaddrinfo(addr, COORDINATOR_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET ConnectSocket = INVALID_SOCKET;

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	m_recievePacketsThread = new std::thread([&]() {recievePackets(); });


	//std::string maxGamesText = MAXGAMESMSG + '\3';
	const int BUFFER_LENGTH = 512;
	char sendBuff[BUFFER_LENGTH];
	strcpy_s(sendBuff, BUFFER_LENGTH,"TEST");
	//sendBuff = MAXGAMESMSG.c_str();
	if (!sendData(ConnectSocket, sendBuff, (int)strlen(sendBuff))) {
		return false;
	}

	std::cout << "Sent " << (int)strlen(sendBuff) << " bytes." << std::endl;
	
	strcpy_s(sendBuff, BUFFER_LENGTH,"I am a server\n");
	sendData(ConnectSocket, sendBuff, (int)strlen(sendBuff));
	while (true) {
		Sleep(10000);
		//std::string sendStr;
		//std::cout << "Message: ";

		//char inputBuffer[512];
		////std::cin.getline(inputBuffer, 512);

		////sendStr = inputBuffer;
		//sendStr = "Beep!\n";

		//strcpy_s(sendBuff, BUFFER_LENGTH,sendStr.c_str());

		//
		//if (!sendData(ConnectSocket, sendBuff, (int)strlen(sendBuff))) {
		//	break;
		//}
		//

		//std::cout << "Sent " << (int)strlen(sendBuff) << " bytes." << std::endl;
		//std::cout << WSAGetLastError << std::endl;
	}

	std::cout << "Connection Lost.";

	return true;
}

bool Server::recievePackets()
{
	SOCKET s;
	struct sockaddr_in6 server, si_other;
	//struct addrinfo *result, hints;

	//ZeroMemory(&hints, sizeof(hints));
	//hints.ai_family = AF_INET6;
	//hints.ai_socktype = SOCK_DGRAM;
	//hints.ai_protocol = IPPROTO_UDP;
	//hints.ai_flags = AI_PASSIVE;

	int slen, recv_len;	

	slen = sizeof(si_other);

	//Create a socket
	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf(" Could not create socket : % d ", WSAGetLastError());
	}
	printf(" Socket created.\n ");

	//Prepare the sockaddr_in structure
	//getaddrinfo(NULL, CLIENT_PORT, &hints, &result);
	memset(&server, 0, sizeof(server));
	server.sin6_family = AF_INET6;
	server.sin6_addr = in6addr_any;
	server.sin6_port = htons(CLIENT_PORT);

	//Bind
	if (bind(s, (struct sockaddr*) &server, sizeof(server)) == SOCKET_ERROR)
	//if (bind(s, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR)
	{
		printf(" Bind failed with error code : % d ", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	char buf[BUFFER_SIZE];
	//keep listening for data
	int counter = 0;
	while (true)
	{
		//fflush(stdout);

		//clear the buffer by filling null, it might have previously received data
		memset(buf, 0, BUFFER_SIZE);

		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(s, buf, BUFFER_SIZE, 0, (struct sockaddr*) &si_other, &slen)) == SOCKET_ERROR)
		{
			printf(" recvfrom() failed with error code : % d ", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		//print details of the client/peer and the data received
		if (counter == 50) {
			char* buff = new char[32];
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

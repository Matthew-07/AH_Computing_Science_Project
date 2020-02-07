#include "Server.h"

bool Server::init() {
	WSADATA wsaData;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return false;
	}

	// Read settings
	std::string text;
	std::fstream f("serverconfig.txt",std::ios::in);
	if (f.is_open()) {
		getline(f, text);
		MAXGAMES = stoi(text);
		MAXGAMESMSG = text;
	} else {
		f = std::fstream("serverconfig.txt", std::ios::out);
		f << 8;
		MAXGAMES = 8;
		MAXGAMESMSG = "8";
	}
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

	// Resolve the server address and port
	iResult = getaddrinfo("DESKTOP-BJ9V93R", COORDINATOR_PORT, &hints, &result);
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


	//std::string maxGamesText = MAXGAMESMSG + '\3';
	const int BUFFER_LENGTH = 512;
	char sendBuff[BUFFER_LENGTH];
	strcpy_s(sendBuff, BUFFER_LENGTH,MAXGAMESMSG.c_str());
	//sendBuff = MAXGAMESMSG.c_str();
	if (!sendData(ConnectSocket, sendBuff, (int)strlen(sendBuff))) {
		return false;
	}

	std::cout << "Sent " << (int)strlen(sendBuff) << " bytes." << std::endl;
	
	strcpy_s(sendBuff, BUFFER_LENGTH,"I am a server\n");
	sendData(ConnectSocket, sendBuff, (int)strlen(sendBuff));
	while (true) {
		std::string sendStr;
		std::cout << "Message: ";

		char inputBuffer[512];
		std::cin.getline(inputBuffer, 512);

		sendStr = inputBuffer;
		sendStr += '\n';

		strcpy_s(sendBuff, BUFFER_LENGTH,sendStr.c_str());

		
		if (!sendData(ConnectSocket, sendBuff, (int)strlen(sendBuff))) {
			break;
		}
		

		std::cout << "Sent " << (int)strlen(sendBuff) << " bytes." << std::endl;
		std::cout << WSAGetLastError << std::endl;
	}

	std::cout << "Connection Lost.";

	return true;
}
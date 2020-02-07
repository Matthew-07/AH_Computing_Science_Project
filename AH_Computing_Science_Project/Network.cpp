#include "Network.h"

bool Network::init() {
	WSADATA wsaData;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		MessageBoxA(NULL, "Failed to connect to server.", "Error", NULL);
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


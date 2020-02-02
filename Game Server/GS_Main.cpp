#include "GS_pch.h"
#include "Server.h"

int main() {
	Server gameServer;
	if (!gameServer.init()) {
		std::cout << "Failed to initalize Winsock.";
		return 1;
	}
	if (!gameServer.start()) {
		std::cout << "Failed to start game server.";
		return 1;
	}

	getchar();
}
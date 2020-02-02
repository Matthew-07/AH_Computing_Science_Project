#include "GC_pch.h"
#include "Coordinator.h"
#include "Database.h"

int main() {
	std::cout << "Initialising Game Coordinator." << std::endl;
	std::cout << "------------------------------" << std::endl 
		<< std::endl;

	// To do - connect to database
	Database myDatabase;
	if (!myDatabase.init()) {
		return 1;
	}
	std::cout << "Connected to database" << std::endl;
	
	Coordinator gameCoordinator;
	
	if (!gameCoordinator.init()) {
		std::cout << "Failed to initalize winsock.";
		return 1;
	}

	if (!gameCoordinator.startServer()) {
		std::cout << "Failed to start server.";
		return 1;
	}

	if (!gameCoordinator.run()) {
		return 1;
	}
	return 0;
}
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
		getchar();
		return 1;
	}

	myDatabase.addUser("test", "password");

	std::cout << "Connected to database" << std::endl;
	
	Coordinator gameCoordinator = Coordinator(&myDatabase);
	
	if (!gameCoordinator.init()) {
		std::cout << "Failed to initalize winsock.";
		getchar();
		return 1;
	}

	if (!gameCoordinator.startServer()) {
		std::cout << "Failed to start server.";
		getchar();
		return 1;
	}

	if (!gameCoordinator.run()) {
		getchar();
		return 1;
	}
	return 0;
}
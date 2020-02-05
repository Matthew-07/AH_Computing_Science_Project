#include "Database.h"

bool Database::init() {
	int result = sqlite3_open_v2("Users.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	if (result != 0) {
		// Database failed to open
		sqlite3_close_v2(db);
		return false;
	}

	if (!loadQuery("CreateTable.sql", q_createTable)) {
		std::cout << "Failed to load 'checkUsername' query." << std::endl;
		return false;
	}
	
	char* err;
	sqlite3_exec(db, q_createTable.c_str(), NULL, NULL, &err);

	if (err != NULL) {
		// Failed to run query
		std::cout << "Failed to create table." << std::endl;
		return false;
	}
	

	// -- Load Queries --
	// Check Username
	if (!loadQuery("checkUsername.sql", q_checkUsername)) {
		std::cout << "Failed to load 'checkUsername' query." << std::endl;
		return false;
	}

	//std::cout << q_checkUsername << std::endl;

	// Create Account

	if (!loadQuery("CreateAccount.sql", q_createAccount)) {
		std::cout << "Failed to load 'createAccount' query." << std::endl;
		return false;
	}

	//std::cout << q_createAccount << std::endl;

	// Check Login

	if (!loadQuery("CheckLogIn.sql", q_checkLogIn)) {
		std::cout << "Failed to load 'checkLogIn' query." << std::endl;
		return false;
	}

	//std::cout << q_checkLogIn << std::endl;

	return true;
}

int Database::addUser(std::string username, std::string password)
{
	char buff[512]; // Buffer must be large enough to hold entire query

	sprintf_s(buff, q_checkUsername.c_str(), username);

	char* err = NULL;
	bool foundRows = false;

	sqlite3_exec(db, buff, foundRowsCallback, &foundRows, &err);
	if (err != NULL) {
		// Failed to run query
		std::cout << buff << "\n" << err << "\n";
		return -1;
	}

	if (foundRows) {
		std::cout << "Username in use.";
		return -1;
	}

	sprintf_s(buff, q_createAccount.c_str(), username, password);

	sqlite3_exec(db, buff, NULL, NULL, &err);
	if (err != NULL) {
		// Failed to run query
		std::cout << buff << "\n" << err << "\n";
		return -1;
	}

	std::cout << "Account created.";

	return logIn(username,password);
}

int Database::logIn(std::string username, std::string password) {
	char buff[512]; // Buffer must be large enough to hold entire query

	sprintf_s(buff, q_checkLogIn.c_str(), username, password);

	char* err = NULL;
	int userId = -2;

	sqlite3_exec(db, buff, returnIntCallback, &userId, &err);

	std::cout << std::endl << "Buffer: " << buff << std::endl;

	if (err != NULL) {
		// Failed to run query
		std::cout << buff << "\n" << err << "\n";
		return -3;
	}

	return userId;
}

Database::~Database()
{
	sqlite3_close_v2(db);
}

int Database::foundRowsCallback(void* a_param, int argc, char** argv, char** column)
{
	*reinterpret_cast<bool*>(a_param) = true;
	return 0;
}

int Database::returnIntCallback(void* a_param, int argc, char** argv, char** column)
{
	*reinterpret_cast<int*>(a_param) = std::atoi(argv[0]);
	return 0;
}

bool Database::loadQuery(std::string path, std::string &var)
{
	std::ostringstream sstream;
	std::ifstream fs(path);

	if (!fs.is_open())
	{
		return false;
	}

	sstream << fs.rdbuf();
	var = std::string(sstream.str());

	fs.close();
	
	return true;
}

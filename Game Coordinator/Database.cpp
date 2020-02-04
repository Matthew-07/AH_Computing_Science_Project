#include "Database.h"

bool Database::init() {
	int result = sqlite3_open_v2("Users.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	if (result != 0) {
		// Database failed to open
		sqlite3_close_v2(db);
		return false;
	}
	
	std::ostringstream sstream;
	std::ifstream fs("createTable.sql");

	if (!fs.is_open())
	{
		std::cout << "Failed to load query." << std::endl;
		return false;
	}

	sstream << fs.rdbuf();
	std::string str(sstream.str());
	const char* ptr = str.c_str();
	
	char* err;
	result = sqlite3_exec(db, ptr, NULL, NULL, &err);
	if (result != 0) {
		// Failed to create table
		sqlite3_close_v2(db);
		return false;
	}

	if (err != NULL) {
		// Failed to run query
		std::cout << "Failed to create table." << std::endl;
		return false;
	}

	return true;
}

bool Database::addUser(std::string username, std::string password)
{
	std::ostringstream sstream;
	std::ifstream fs("checkUsername.sql");

	if (!fs.is_open())
	{
		std::cout << "Failed to load query." << std::endl;
		return false;
	}

	sstream << fs.rdbuf();
	std::string str(sstream.str());
	const char* ptr = str.c_str();

	std::cout << ptr << "\n";

	char buff[512];

	sprintf_s(buff, ptr, username, password);

	fs.close();
	sstream.clear();

	char* err;
	sqlite3_exec(db, buff, foundRowsCallback, this, &err);
	if (err != NULL) {
		// Failed to run query
		std::cout << ptr << "\n" << buff << "\n" << err << "\n";
		return false;
	}

	if (foundRows) {
		std::cout << "Username in use.";
		foundRows = false;
		return false;
	}

	fs.clear();
	fs.open("createAccount.sql");

	if (!fs.is_open())
	{
		std::cout << "Failed to load query." << std::endl;
		return false;
	}

	sstream << fs.rdbuf();
	std::string str2(sstream.str());
	const char* ptr2 = str2.c_str();

	sprintf_s(buff, ptr2, username, password);

	fs.close();
	sstream.clear();

	sqlite3_exec(db, buff, NULL, NULL, &err);
	if (err != NULL) {
		// Failed to run query
		std::cout << ptr2 << "\n" << err << "\n";
		return false;
	}

	return true;
}

Database::~Database()
{
	sqlite3_close_v2(db);
}

int Database::foundRowsCallback(void* a_param, int argc, char** argv, char** column)
{
	static_cast<Database*>(a_param)->foundRows = true;
	return 0;
}

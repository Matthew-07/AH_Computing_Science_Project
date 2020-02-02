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
	sstream << fs.rdbuf();
	const std::string str(sstream.str());
	const char* ptr = str.c_str();
	

	result = sqlite3_exec(db, ptr, NULL, NULL, NULL);
	if (result != 0) {
		// Failed to create table
		sqlite3_close_v2(db);
		return false;
	}

	return true;
}

Database::~Database()
{
	sqlite3_close_v2(db);
}

#pragma once

#include "GC_pch.h"

// A class for interacting with the database

class Database
{
public:
	bool init();

	bool addUser(std::string username, std::string password);

	~Database();
	
private:
	sqlite3 *db = NULL;

	static int foundRowsCallback(void* a_param, int argc, char** argv, char** column); // After using callback set "foundRows" back to false
	bool foundRows = false;


};


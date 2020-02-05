#pragma once

#include "GC_pch.h"

// A class for interacting with the database

class Database
{
public:
	bool init();

	int addUser(std::string username, std::string password);
	int logIn(std::string username, std::string password);

	~Database();
	
private:
	sqlite3 *db = NULL;

	static int foundRowsCallback(void* a_param, int argc, char** argv, char** column); // After using callback set "foundRows" back to false

	static int returnIntCallback(void* a_param, int argc, char** argv, char** column);

	bool loadQuery(std::string path, std::string &var);

	// Queries
	std::string q_createTable;
	std::string q_checkUsername;
	std::string q_createAccount;
	std::string q_checkLogIn;


};


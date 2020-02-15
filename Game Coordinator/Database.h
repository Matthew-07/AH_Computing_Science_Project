#pragma once

#include "GC_pch.h"

// A class for interacting with the database

//int* teams, int** participants, int numberOfTeams, int* numberOfParticipants

// Use a struct to pass information about the game to make code more readable
struct GameInfo {
public:
	int* teams;
	int* scores;
	int numberOfTeams;


	int** participants;	
	int* numberOfParticipants; // Array storing the number of participants in each team.


	int gameDuration;
	char * date;
};

class Database
{
public:
	bool init();

	int addUser(std::string username, std::string password);
	int logIn(std::string username, std::string password);
	bool addGame(GameInfo &info);

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

	std::string q_addGame;
	std::string q_addTeam;
	std::string q_addParticipant;


};


#pragma once

#include "GC_pch.h"

// A class for interacting with the database

//int* teams, int** participants, int numberOfTeams, int* numberOfParticipants

// Use a struct to pass information about the game to make code more readable
struct GameInfo {
public:
	int32_t* scores;
	int32_t numberOfTeams;


	int32_t** participants;	
	int32_t* numbersOfParticipants; // Array storing the number of participants in each team.


	int32_t duration;
	char date[11]; // 8 digits, 2 '/' chars and a null character
};

class Database
{
public:
	bool init();

	int32_t addUser(std::string username, std::string password);
	int32_t logIn(std::string username, std::string password);
	bool addGame(GameInfo &info);
	bool getUserGameInfo(int32_t userId, int32_t* numberOfGames, int32_t* numberOfWins);

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

	std::string q_countGames;
	std::string q_countWins;


};


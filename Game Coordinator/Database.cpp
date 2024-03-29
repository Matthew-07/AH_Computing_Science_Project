#include "Database.h"

bool Database::init() {

	int result = sqlite3_open_v2("Users.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

	if (result != 0) {
		// Database failed to open
		sqlite3_close_v2(db);
		return false;
	}

	if (!loadQuery("CreateTable.sql", q_createTable)) {
		std::cout << "Failed to load 'checkUsername' query." << std::endl << std::endl;
		return false;
	}
	
	char* err;
	sqlite3_exec(db, q_createTable.c_str(), NULL, NULL, &err);

	if (err != NULL) {
		// Failed to run query
		std::cout << "Failed to create table." << std::endl << std::endl;
		std::cout << err << std::endl;
		return false;
	}
	

	// -- Load Queries --
	// Check Username
	if (!loadQuery("checkUsername.sql", q_checkUsername)) {
		std::cout << "Failed to load 'checkUsername' query." << std::endl << std::endl;
		return false;
	}

	// Create Account

	if (!loadQuery("CreateAccount.sql", q_createAccount)) {
		std::cout << "Failed to load 'createAccount' query." << std::endl << std::endl;
		return false;
	}

	// Check Login

	if (!loadQuery("CheckLogIn.sql", q_checkLogIn)) {
		std::cout << "Failed to load 'checkLogIn' query." << std::endl << std::endl;
		return false;
	}

	// Add Game

	if (!loadQuery("addGame.sql", q_addGame)) {
		std::cout << "Failed to load 'addGame' query." << std::endl << std::endl;
		return false;
	}

	// Add Team

	if (!loadQuery("addTeam.sql", q_addTeam)) {
		std::cout << "Failed to load 'addTeam' query." << std::endl << std::endl;
		return false;
	}

	// Add Participant

	if (!loadQuery("addParticipant.sql", q_addParticipant)) {
		std::cout << "Failed to load 'addParticipant' query." << std::endl << std::endl;
		return false;
	}

	// Count Games
	if (!loadQuery("countGames.sql", q_countGames)) {
		std::cout << "Failed to load 'countGame' query." << std::endl << std::endl;
		return false;
	}

	// Count Wins
	if (!loadQuery("countWins.sql", q_countWins)) {
		std::cout << "Failed to load 'countWins' query." << std::endl << std::endl;
		return false;
	}

	return true;
}

int32_t Database::addUser(std::string username, std::string password)
{
	// Create a temporary connection for insert
	sqlite3 *tempdb;
	sqlite3_open_v2("Users.db", &tempdb, SQLITE_OPEN_READWRITE, NULL);

	char buff[512]; // Buffer must be large enough to hold entire query

	sprintf_s(buff, q_checkUsername.c_str(), username.c_str());

	char* err = NULL;
	bool foundRows = false;

	sqlite3_exec(tempdb, buff, foundRowsCallback, &foundRows, &err);
	if (err != NULL) {
		// Failed to run query
		std::cout << buff << "\n" << err << "\n";
		return -1;
	}

	if (foundRows) {
		std::cout << "Username " << username << " in use." << std::endl << std::endl;
		return -1;
	}

	sprintf_s(buff, q_createAccount.c_str(), username.c_str(), password.c_str());

	sqlite3_exec(tempdb, buff, NULL, NULL, &err);
	if (err != NULL) {
		// Failed to run query
		std::cout << buff << "\n" << err << "\n";
		return -1;
	}

	std::cout << "Account created." << std::endl << std::endl;

	int userId = sqlite3_last_insert_rowid(tempdb);
	sqlite3_close_v2(tempdb);
	return userId;
}

int32_t Database::logIn(std::string username, std::string password) {
	char buff[512]; // Buffer must be large enough to hold entire query

	sprintf_s(buff, q_checkLogIn.c_str(), username.c_str(), password.c_str());
	
	char* err = NULL;
	int userId = -2;

	sqlite3_exec(db, buff, returnIntCallback, &userId, &err);

	//std::cout << std::endl << "Buffer: " << buff << std::endl;

	if (err != NULL) {
		// Failed to run query
		std::cout << buff << "\n" << err << "\n";
		return -3;
	}

	return userId;
}

bool Database::addGame(GameInfo &info)
{
	// Create a temporary connection for insert
	sqlite3* tempdb;
	sqlite3_open_v2("Users.db", &tempdb, SQLITE_OPEN_READWRITE, NULL);

	char buff[512];
	char* err;

	sprintf_s(buff, q_addGame.c_str(), info.date, info.duration);
	sqlite3_exec(tempdb, buff, NULL, NULL, &err);

	if (err != NULL) {
		// Failed to run query
		std::cout << buff << "\n" << err << "\n";
		return false;
	}

	// Clear buffer
	ZeroMemory(buff,512);

	int gameId = sqlite3_last_insert_rowid(tempdb);

	for (int t = 0; t < info.numberOfTeams; t++) {
		sprintf_s(buff, q_addTeam.c_str(), gameId, info.scores[t]);

		sqlite3_exec(tempdb, buff, NULL, NULL, &err);

		if (err != NULL) {
			// Failed to run query
			std::cout << buff << "\n" << err << "\n";
			return false;
		}

		int teamId = sqlite3_last_insert_rowid(tempdb);

		for (int p = 0; p < info.numbersOfParticipants[t]; p++) {
			ZeroMemory(buff, 512);
			sprintf_s(buff, q_addParticipant.c_str(), info.participants[t][p], teamId);

			sqlite3_exec(tempdb, buff, NULL, NULL, &err);

			if (err != NULL) {
				// Failed to run query
				std::cout << buff << "\n" << err << "\n";
				return false;
			}
		}

		ZeroMemory(buff, 512);
	}

	return true;
}

bool Database::getUserGameInfo(int32_t userId, int32_t *numberOfGames, int32_t *numberOfWins)
{
	char buff[512]; // Buffer must be large enough to hold entire query

	sprintf_s(buff, q_countGames.c_str(), userId);

	char* err = NULL;

	sqlite3_exec(db, buff, returnIntCallback, numberOfGames, &err);

	if (err != NULL) {
		// Failed to run query
		std::cout << buff << "\n" << err << "\n";
		return false;
	}

	ZeroMemory(buff, 512);

	sprintf_s(buff, q_countWins.c_str(), userId);

	err = NULL;

	sqlite3_exec(db, buff, returnIntCallback, numberOfWins, &err);

	if (err != NULL) {
		// Failed to run query
		std::cout << buff << "\n" << err << "\n";
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

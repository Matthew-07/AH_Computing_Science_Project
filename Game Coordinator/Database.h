#pragma once

#include "GC_pch.h"

// A class for interacting with the database

class Database
{
public:
	bool init();

	~Database();
	
private:
	sqlite3 *db = NULL;
};


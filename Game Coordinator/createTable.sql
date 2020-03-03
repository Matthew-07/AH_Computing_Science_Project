CREATE TABLE IF NOT EXISTS users(
id INTEGER PRIMARY KEY,
username TEXT NOT NULL,
password TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS games(
id INTEGER PRIMARY KEY,
datePlayed TEXT NOT NULL,
duration INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS teams(
id INTEGER PRIMARY KEY,
gameId INTEGER,
score INTEGER NOT NULL,
FOREIGN KEY (gameId) REFERENCES games(id)
);

CREATE TABLE IF NOT EXISTS particpations(
playerId INTEGER,
teamId INTEGER,
PRIMARY KEY (playerID,teamID),
FOREIGN KEY (playerID) REFERENCES users(id),
FOREIGN KEY (teamId) REFERENCES teams(id)
);
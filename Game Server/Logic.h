#pragma once

#include "GS_pch.h"
#include "GameConstants.h"

// Data that is sent to the client
struct PlayerData {
public:
	float pos[2] = { 0,0 };
	float targetPos[2] = { 0,0 };

	int32_t shieldDuration = 0;
	int32_t stoneDuration = 0;

	int32_t cooldowns[5] = { 0,0,0,0,0 };
	int32_t id = -1;	
	int32_t team = -1;
};

// Struct for storing information about the player
struct Player {
public:	
	bool isAlive;

	float oldPos[2] = { 0,0 };

	PlayerData data = PlayerData();	
};

struct ShockwaveData {
	float pos[2] = { 0,0 };
	float dest[2] = { 0,0 };
	float start[2] = { 0,0 };

	int32_t team = -1;
};

struct Shockwave {
	float oldPos[2] = { 0,0 };
	ShockwaveData data = ShockwaveData();	
};

struct DaggerData {
	float pos[2] = { 0,0 };
	int32_t targetId = -1;	

	int32_t team = -1;
};

struct Dagger {
	float oldPos[2] = { 0,0 };
	DaggerData data = DaggerData();	
	int32_t lifetime = 0;

	int32_t senderId = -1;
};

union data64 {
	float f[2];
	int32_t i[2] = { 0,0 };
};

struct Input {
public:
	int32_t playerId = -1;
	int32_t type = -1;
	data64 data = data64(); // Up to two 4 byte pieces of data
};

struct GameResult {
	int32_t duration;
	int32_t numberOfTeams;
	int32_t* teamScores = NULL;
	int32_t* numbersOfParticipants = NULL;
	int32_t** participantIDs = NULL;
};

// Each 'Logic' Object will represent a game currently being played.
class Logic
{
public:
	Logic(int32_t numberOfPlayers, int32_t numberOfTeams, int32_t* playerIds, int32_t* playerTeams);

	// Returns the index of the winning team if the game finished, -2 if the round finished else -1.
	int32_t tick(std::list<Input> &playerInputs);
	
	int32_t getMaxGamestateSize();

	// Fills the buffer with the gamestate and returns the amount of space used.
	int32_t getGamestate(char * buffer); 

	int32_t index;
	bool checkForUser(int32_t userId) {
		for (int p = 0; p < m_numberOfPlayers; p++) {
			if (m_players[p].data.id == userId) {
				return true;
			}
		}
		return false;
	}

	int32_t getNumberOfPlayers() { return m_numberOfPlayers; }
	int32_t getNumberOfTeams() { return m_numberOfTeams; }
	int32_t* getPlayerIds() { return m_playerIds; }
	int32_t* getPlayerTeams() { return m_playerTeams; }	

	void endGame(GameResult &results);

private:
	void startRound();	

	bool calculateCollision(float* pos1, float* mov1, float* pos2, float* mov2, float maxDist);
	void restrictBounds(float* startPos, float* endPos) {
		if (sqrt(pow(endPos[0], 2) + pow(endPos[1], 2)) > (m_mapSize - PLAYER_WIDTH)) {

			if (endPos[0] == startPos[0]) {
				// Player moved vertically, slightly adjust startPos so calculation works
				startPos[0] += 1 / 10000;
			}

			float m = (startPos[1] - endPos[1]) / (startPos[0] - endPos[0]);
			float yintercept = startPos[1] - m * startPos[0];

			float a = pow(m, 2) + 1;
			float b = 2 * m * yintercept;
			float c = pow(yintercept,2) - pow(m_mapSize - PLAYER_WIDTH, 2);

			float discrminant = sqrt(pow(b, 2) - 4 * a * c);
			float root1 = (-b - discrminant) / (2 * a);

			float newX, newY;

			if (endPos[0] < root1) {
				newX = root1;
			}
			else {
				newX = (-b + discrminant) / (2 * a);
			}

			newY = m * newX + yintercept;

			endPos[0] = newX;
			endPos[1] = newY;

		}
	}

	int32_t getPlayerById(int32_t id);


	int32_t m_mapSize;
	int32_t m_numberOfPlayers;
	int32_t m_numberOfTeams;

	int32_t* m_playerIds, * m_playerTeams;

	int32_t* m_teamScores;

	Player* m_players;

	int32_t tickNumber;

	std::list<Shockwave> * m_shockwaves;
	std::list<Dagger> * m_daggers;
};

inline void calculateMovement(float* pos, float* targetPos, float speed)
{
	if (speed == 0) {
		return;
	}

	float xDiff = targetPos[0] - pos[0];
	float yDiff = targetPos[1] - pos[1];

	float dist = sqrt(pow(xDiff, 2) + pow(yDiff, 2));

	if (dist < speed) {
		pos[0] = targetPos[0];
		pos[1] = targetPos[1];
		return;
	}

	pos[0] += xDiff / dist * speed;
	pos[1] += yDiff / dist * speed;
}

inline float calculateDistance(float* point1, float* point2)
{
	return sqrt(pow(point2[0] - point1[0], 2) + pow(point2[1] - point1[1], 2));
}

/* Tasks per tick:

1. Move players and projectiles
2. Check if any players were killed
3. Check if the round ended
4. Handle player input
5. Send packet with the current gamestate to clients

1.1 Move players and 'shockwaves' based on their direction
1.2 Move daggers towards their target.
1.3 The order shall be daggers -> shockwaves -> players

2.1 Check if any projectiles intersect with players
2.2 Check if the team of the player is different to the team of the projectile, if so the player was killed.

3.1 Check that atleast 2 teams have >= 1 player who hasn't been killed
3.2 If not, the round has ended. Whichever team has players left gains a point or if no players do it was a draw so no score is added.

4.1 Iterate through player input
4.2 If the input was to move toward a point set target point as appropriate.
4.3 If it was to use an ability check that it is not on cooldown, if not then use it.

5.1 The getGamestate() function will be called, fill the given buffer with the gamestate
5.2 While doing so count the number of bytes written and return this.
*/

/* Information that makes up the Gamestate:
Player coordinates and target point (the client will extrapolate extra frames when the packet is late by assuming
all players just keep towards the target point).

Which defensive abilities are active and how their remaining duration.

Player Cooldowns

Projectile coordinates and types (the movement of projectiles will be calculated client side when extrapolating extra frames).

Other things can instead be sent via TCP such as the score.
*/

/* Input Types
Player Move -> Players is to move towards a certain point in a straight line until it is reached, cancels Stone Form
x,y coordinate

Shield -> Activate shield

Blink -> Set player coordinates to a certain point
x,y coordinate

Dagger -> Use dagger of enemy player nearest to point it is cast
x,y coordinate

Stone Form -> Activate Stone Form

Attack -> Attack enemy players in direction
direction (either angle or unit vector);

*/
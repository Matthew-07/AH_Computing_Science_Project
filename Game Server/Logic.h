#pragma once

#include "GS_pch.h"

// Player Inputs
#define INP_MOVE	0x01
#define	INP_SHIELD	0x02
#define INP_BLINK	0x03
#define INP_SHOCK	0x04
#define	INP_DAGGER	0x05
#define	INP_STONE	0x06
#define	INP_ATTACK	0x07

#define SHIELD_INDEX	0
#define BLINK_INDEX		1
#define SHOCK_INDEX		2
#define DAGGER_INDEX	3
#define STONE_INDEX		4
#define ATTACK_INDEX	5

// Data that is sent to the client
struct PlayerData {
public:
	float pos[2];
	float targetPos[2];

	int32_t shieldDuration;
	int32_t stoneDuration;

	int32_t cooldowns[6];
	int32_t id;	
	int32_t team;
};

// Struct for storing information about the player
struct Player {
public:	
	bool isAlive;

	float oldPos[2];

	PlayerData data;	
};

struct ShockwaveData {
	float pos[2];
	float dest[2];

	int32_t team;
};

struct Shockwave {
	float oldPos[2];
	ShockwaveData data;	
};

struct DaggerData {
	float pos[2];
	int32_t targetId;
	int32_t senderId;
	int32_t lifetime;

	int32_t team;
};

struct Dagger {
	float oldPos[2];
	DaggerData data;	
};

struct Input {
public:
	int32_t playerId;
	int32_t type;
	float data[2]; // Up to two bits of data
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

	bool join() {

	}

private:
	void startRound();	

	bool calculateCollision(float* pos1, float* mov1, float* pos2, float* mov2, float maxDist);

	int32_t getPlayerById(int32_t id);


	int32_t m_mapSize;
	int32_t m_numberOfPlayers;
	int32_t m_numberOfTeams;

	int32_t* m_teamScores;

	Player* m_players;

	std::list<Shockwave> m_shockwaves;
	std::list<Dagger> m_daggers;
};

inline void calculateMovement(float* pos, float* targetPos, int32_t speed)
{
	float xDiff = targetPos[0] - pos[0];
	float yDiff = targetPos[1] - pos[1];

	float dist = sqrt(pow(xDiff, 2) + pow(yDiff, 2));

	if (dist < speed) {
		pos[0] = targetPos[0];
		pos[1] = targetPos[1];
		return;
	}

	pos[0] = xDiff / dist * speed;
	pos[1] = yDiff / dist * speed;
}

inline float calculateDistance(float* point1, float* point2)
{
	return sqrt(pow(point2[0] - point1[0], 2) + pow(point2[1] - point1[1], 2));
}

const int32_t	TICKS_PER_SECOND = 64;

const int32_t	MAX_SCORE = 15;

const float	PLAYER_SPEED = 10;

const float	SHOCKWAVE_SPEED = 50;
const int32_t	SHOCKWAVE_RANGE = 1000;
const int32_t	SHOCKWAVE_COLLISION_SIZE = 35;
const int32_t	SHOCKWAVE_COOLDOWN = 80;
// A shockwave takes range/speed time to finish and can at most be produced once every cooldown ticks.
const int32_t	MAX_SHOCKWAVES = ceil(SHOCKWAVE_RANGE / SHOCKWAVE_SPEED / SHOCKWAVE_COOLDOWN);

const float		DAGGER_SPEED = 35;
const int32_t	DAGGER_RANGE = 800;
const int32_t	DAGGER_MAX_LIFETIME = 600;
const int32_t	DAGGER_COLLISION_SIZE = 20;
const int32_t	DAGGER_COOLDOWN = 100;
const int32_t	MAX_DAGGERS = DAGGER_MAX_LIFETIME / DAGGER_COOLDOWN;

const int32_t	SHIELD_DURATION = 6;
const int32_t	SHIELD_COOLDOWN = 80;

const int32_t	BLINK_COOLDOWN = 60;

const int32_t	STONE_DURATION = 150;
const int32_t	STONE_COOLDOWN = 300;

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
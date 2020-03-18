#include "Logic.h"

Logic::Logic(int32_t numberOfPlayers, int32_t numberOfTeams, int32_t* playerIds, int32_t* playerTeams) {

	m_numberOfPlayers = numberOfPlayers;
	m_numberOfTeams = numberOfTeams;
	m_players = new Player[numberOfPlayers];

	m_teamScores = new int32_t[numberOfTeams];
	for (int t = 0; t < numberOfTeams; t++) {
		m_teamScores[t] = 0;
	}

	// At the moment completely arbitrary
	m_mapSize = numberOfPlayers * MAP_SIZE_PER_PLAYER + MAP_SIZE_CONSTANT;

	for (int p = 0; p < numberOfPlayers; p++) {
		m_players[p].data.id = playerIds[p];
		m_players[p].data.team = playerTeams[p];
	}

	m_playerIds = playerIds;
	m_playerTeams = playerTeams;

	tickNumber = 0;

	m_daggers = new std::list<Dagger>();

	m_shockwaves = new std::list<Shockwave>();

	startRound();

}

int32_t Logic::tick(std::list<Input> &playerInputs)
{
	//1. Move players and projectiles and 2. TODO Check if any players were killed
	// Daggers
	if (m_daggers->size() > 0) {
		std::list<Dagger>::iterator dagger = m_daggers->begin();
		while (dagger != m_daggers->end()) {

			dagger->lifetime++;
			if (dagger->lifetime > DAGGER_MAX_LIFETIME) {
				dagger = m_daggers->erase(dagger);
				continue;
			}

			for (int p = 0; p < m_numberOfPlayers; p++) {
				if (m_players[p].data.id == dagger->data.targetId) {
					if (m_players[p].isAlive) {

						dagger->oldPos[0] = dagger->data.pos[0];
						dagger->oldPos[1] = dagger->data.pos[1];

						calculateMovement(dagger->data.pos, m_players[p].data.pos, DAGGER_SPEED_PER_TICK);
					}
					else {
						dagger = m_daggers->erase(dagger);
						continue;
					}
					break;
				}
			}
			dagger++;
		}
	}

	if (m_shockwaves->size() > 0) {
		std::list<Shockwave>::iterator shockwave = m_shockwaves->begin();
		while (shockwave != m_shockwaves->end()) {

			shockwave->oldPos[0] = shockwave->data.pos[0];
			shockwave->oldPos[1] = shockwave->data.pos[1];

			calculateMovement(shockwave->data.pos, shockwave->data.dest, SHOCKWAVE_SPEED_PER_TICK);

			if (shockwave->data.pos[0] == shockwave->data.dest[0]) {
				shockwave = m_shockwaves->erase(shockwave);
			}
			else {
				shockwave++;
			}
		}
	}

	for (int p = 0; p < m_numberOfPlayers; p++) {
		if (m_players[p].isAlive) {

			if (m_players[p].data.stoneDuration == 0) {

				m_players[p].oldPos[0] = m_players[p].data.pos[0];
				m_players[p].oldPos[1] = m_players[p].data.pos[1];

				calculateMovement(m_players[p].data.pos, m_players[p].data.targetPos, PLAYER_SPEED_PER_TICK);

				// Check if the player has been killed, the collision detection for the shockwave is not necessary if the player is invulnerable.
				if (m_players[p].data.shieldDuration == 0 && m_shockwaves->size() > 0) {

					for (auto shockwave : *m_shockwaves) {
						if (calculateCollision(
							m_players[p].oldPos,
							m_players[p].data.pos,
							shockwave.oldPos,
							shockwave.data.pos,
							SHOCKWAVE_COLLISION_SIZE
						)) {
							if (m_players[p].data.team != shockwave.data.team) {
								m_players[p].isAlive = false;
								break;
							}
						}
					}
				}
			}

			if (m_daggers->size() > 0) {
				std::list<Dagger>::iterator dagger = m_daggers->begin();
				while (dagger != m_daggers->end()) {					
					if (calculateCollision(
						m_players[p].oldPos,
						m_players[p].data.pos,
						dagger->oldPos,
						dagger->data.pos,
						DAGGER_COLLISION_SIZE
					))
					{
						if (m_players[p].data.id == dagger->data.targetId) {
							if (m_players[p].data.stoneDuration > 0) {
								dagger = m_daggers->erase(dagger);
							}
							else if (m_players[p].data.shieldDuration == 0) {
								m_players[p].isAlive = false;
							}
							else {
								// When a dagger hits a player with a shield it is reflected back.
								dagger->data.targetId = dagger->senderId;
								dagger->senderId = m_players[p].data.id;

								dagger->data.team = m_players[p].data.team;
							}
							break;
						}
					}
					dagger++;
				}
			}

			// If the player is still alive, reduce all active cooldowns and durations by 1.
			for (int c = 0; c < 5; c++) {
				if (m_players[p].data.cooldowns[c] > 0) {
					m_players[p].data.cooldowns[c]--;
				}
			}

			if (m_players[p].data.shieldDuration > 0) {
				m_players[p].data.shieldDuration--;
			}

			if (m_players[p].data.stoneDuration > 0) {
				m_players[p].data.stoneDuration--;
			}
			
		}
	}

	//3. Check if the round ended
	bool roundEnded = true;
	int32_t winningTeam = -1;
	for (int a = 0; a < m_numberOfPlayers; a++) {
		if (m_players[a].isAlive) {
			winningTeam = m_players[a].data.team;
			for (int b = a + 1; b < m_numberOfPlayers; b++) {
				// If the second player is alive and a different team to the first the round has not ended.
				if (m_players[b].isAlive) {
					if (m_players[a].data.team != m_players[b].data.team) {
						roundEnded = false;
						break;
					}
				}
			}
			break;
		}
	}

	if (roundEnded) {
		if (winningTeam >= 0) {
			// Teams should be numbered 0...m_numberOfTeams - 1 and so the team number should simply be the index of the teams score
			// A teams score shouldn't be able to be greater than MAX_SCORE so '==' should also work.
			if (++m_teamScores[winningTeam] >= MAX_SCORE) {
				// The team won
				return winningTeam;
			}
			// Else the game hasn't finished, start a new round
		}
		startRound();
		return -2; // No inputs should be handled as a new round is starting
	}


	// 4. Handle player input

	// IMPORTANT -> when collecting player inputs it is important to somehow check that the input is from the current round, currently it is assumed this is handled elsewhere.
	while (playerInputs.size() > 0) {
		int32_t playerIndex = getPlayerById(playerInputs.front().playerId);
		if (playerIndex < 0 || !m_players[playerIndex].isAlive) {
			playerInputs.pop_front();
			continue;
		}
		switch (playerInputs.front().type) {			
		case INP_MOVE:
		{
			m_players[playerIndex].data.stoneDuration = 0; // Any action cancels stone form
			m_players[playerIndex].data.targetPos[0] = playerInputs.front().data.f[0];
			m_players[playerIndex].data.targetPos[1] = playerInputs.front().data.f[1];	

			restrictBounds(m_players[playerIndex].data.pos, m_players[playerIndex].data.targetPos);

			break;
		}
		case INP_SHIELD:
		{
			if (m_players[playerIndex].data.cooldowns[SHIELD_INDEX] == 0) {
				m_players[playerIndex].data.cooldowns[SHIELD_INDEX] = SHIELD_COOLDOWN;

				m_players[playerIndex].data.stoneDuration = 0; // Any action cancels stone form
				m_players[playerIndex].data.shieldDuration = SHIELD_DURATION;				
			}
			break;
		}
		case INP_BLINK:
		{
			if (m_players[playerIndex].data.cooldowns[BLINK_INDEX] == 0
				&& calculateDistance(m_players[playerIndex].data.pos, playerInputs.front().data.f) < BLINK_RANGE) {	

				restrictBounds(m_players[playerIndex].data.pos, playerInputs.front().data.f);

				m_players[playerIndex].data.stoneDuration = 0; // Any action cancels stone form
				m_players[playerIndex].data.cooldowns[BLINK_INDEX] = BLINK_COOLDOWN;

				m_players[playerIndex].data.pos[0] = playerInputs.front().data.f[0];
				m_players[playerIndex].data.pos[1] = playerInputs.front().data.f[1];

				m_players[playerIndex].data.targetPos[0] = playerInputs.front().data.f[0];
				m_players[playerIndex].data.targetPos[1] = playerInputs.front().data.f[1];

				// Daggers lose their target when their target blinks
				if (m_daggers->size() > 0) {
					std::list<Dagger>::iterator dagger = m_daggers->begin();
					while (dagger != m_daggers->end()) {
						if (dagger->data.targetId == m_players[playerIndex].data.id) {
							dagger = m_daggers->erase(dagger);
						}
						else {
							dagger++;
						}
					}

				}

				// Don't let players use offensive abilities immediately after blinking.
				if (m_players[playerIndex].data.cooldowns[SHOCK_INDEX] < POST_BLINK_COOLDOWN) {
					m_players[playerIndex].data.cooldowns[SHOCK_INDEX] = POST_BLINK_COOLDOWN;
				}
				if (m_players[playerIndex].data.cooldowns[DAGGER_INDEX] < POST_BLINK_COOLDOWN) {
					m_players[playerIndex].data.cooldowns[DAGGER_INDEX] = POST_BLINK_COOLDOWN;
				}
			}
			break;
		}
		case INP_SHOCK:
		{
			if (m_players[playerIndex].data.cooldowns[SHOCK_INDEX] == 0) {

				m_players[playerIndex].data.stoneDuration = 0; // Any action cancels stone form

				m_players[playerIndex].data.cooldowns[SHOCK_INDEX] = SHOCKWAVE_COOLDOWN;

				Shockwave s;

				s.data.pos[0] = m_players[playerIndex].data.pos[0];
				s.data.pos[1] = m_players[playerIndex].data.pos[1];

				s.oldPos[0] = m_players[playerIndex].data.pos[0];
				s.oldPos[1] = m_players[playerIndex].data.pos[1];

				s.data.start[0] = m_players[playerIndex].data.pos[0]; // Used for drawing
				s.data.start[1] = m_players[playerIndex].data.pos[1];

				s.data.dest[0] = playerInputs.front().data.f[0];
				s.data.dest[1] = playerInputs.front().data.f[1];

				// The shockwave should travel SHOCKWAVE_RANGE units
				float dist = calculateDistance(s.data.start, s.data.dest);
				float multiplier = (float) SHOCKWAVE_RANGE / dist;

				s.data.dest[0] = s.data.start[0] + (s.data.dest[0] - s.data.start[0]) * multiplier;
				s.data.dest[1] = s.data.start[1] + (s.data.dest[1] - s.data.start[1]) * multiplier;

				s.data.team = m_players[playerIndex].data.team;

				m_shockwaves->emplace_back(s);
			}
			break;
		}
		case INP_DAGGER:
		{			
			if (m_players[playerIndex].data.cooldowns[DAGGER_INDEX] == 0) {

				m_players[playerIndex].data.stoneDuration = 0; // Any action cancels stone form

				m_players[playerIndex].data.cooldowns[DAGGER_INDEX] = DAGGER_COOLDOWN;

				Dagger d = Dagger();

				d.data.pos[0] = m_players[playerIndex].data.pos[0];
				d.data.pos[1] = m_players[playerIndex].data.pos[1];

				d.oldPos[0] = m_players[playerIndex].data.pos[0];
				d.oldPos[1] = m_players[playerIndex].data.pos[1];

				d.data.targetId = playerInputs.front().data.i[0];
				d.senderId = playerInputs.front().playerId;

				d.lifetime = 0;

				d.data.team = m_players[playerIndex].data.team;

				m_daggers->emplace_back(d);
			}
			break;
		}
		case INP_STONE:
		{
			if (m_players[playerIndex].data.cooldowns[STONE_INDEX] == 0) {
				m_players[playerIndex].data.cooldowns[STONE_INDEX] = STONE_COOLDOWN;

				m_players[playerIndex].data.stoneDuration = STONE_DURATION;
				m_players[playerIndex].data.shieldDuration = 0; // Stone overrides shield.

				// Stop moving
				m_players[playerIndex].data.targetPos[0] = m_players[playerIndex].data.pos[0];
				m_players[playerIndex].data.targetPos[1] = m_players[playerIndex].data.pos[1];
			}
			break;
		}
		}
		playerInputs.pop_front();
	}

	tickNumber++;

	return -1;

	//5. Send packet with the current gamestate to clients - done outside class using other methods.
}

int32_t Logic::getMaxGamestateSize() // The gamestate will not be larger than this
{
	/* The wierd casting is just to avoid compiler warnings about integer overflow 
	as sizeof() returns a 64 bit int.*/
	return (int32_t)
		((UINT64)m_numberOfPlayers * sizeof(PlayerData)
		+ (UINT64)m_numberOfPlayers * MAX_DAGGERS *  ((int32_t) sizeof(DaggerData))
		+ (UINT64)m_numberOfPlayers * MAX_SHOCKWAVES * ((int32_t)sizeof(ShockwaveData))
		+ 16
		+ m_numberOfTeams * sizeof(int32_t));
}

int32_t Logic::getGamestate(char* buffer)
{
	int32_t bytesWritten = 0;

	*(int32_t*)(buffer) = tickNumber;
	bytesWritten += 4; buffer += 4;

	// Number of players
	int32_t numberOfPlayers = 0;
	for (int p = 0; p < m_numberOfPlayers; p++){
		if (m_players[p].isAlive) {
			numberOfPlayers++;
		}
	}
	*(int32_t*)(buffer) = numberOfPlayers;
	bytesWritten += 4; buffer += 4;

	// Player info
	for (int p = 0; p < m_numberOfPlayers; p++) {
		if (m_players[p].isAlive) {
			* (PlayerData*)(buffer) = m_players[p].data;
			bytesWritten += sizeof(PlayerData); buffer += sizeof(PlayerData);
		}
	}	

	int32_t numberOfShockwaves = (int32_t) m_shockwaves->size();
	*(int32_t*)(buffer) = numberOfShockwaves;
	bytesWritten += 4; buffer += 4;

	if (m_shockwaves->size() > 0) {
		for (auto shockwave : *m_shockwaves) {
			*(ShockwaveData*)(buffer) = shockwave.data;
			bytesWritten += sizeof(ShockwaveData); buffer += sizeof(ShockwaveData);
		}
	}

	int32_t numberOfDaggers = (int32_t) m_daggers->size();
	*(int32_t*)(buffer) = numberOfDaggers;
	bytesWritten += 4; buffer += 4;

	if (m_daggers->size() > 0) {
		for (auto dagger : *m_daggers) {
			*(DaggerData*)(buffer) = dagger.data;
			bytesWritten += sizeof(DaggerData); buffer += sizeof(DaggerData);
		}
	}

	for (int t = 0; t < m_numberOfTeams; t++) {
		*(int32_t*)(buffer) = m_teamScores[t];
		bytesWritten += 4; buffer += 4;
	}

	return bytesWritten;
}

void Logic::endGame(GameResult &result)
{
	result.numberOfTeams = m_numberOfTeams;
	result.numbersOfParticipants = new int32_t[m_numberOfTeams];
	result.teamScores =  new int32_t[m_numberOfTeams];

	int32_t* indexInTeam = new int32_t[m_numberOfTeams];

	for (int t = 0; t < m_numberOfTeams; t++) {
		result.numbersOfParticipants[t] = 0;
		result.teamScores[t] = m_teamScores[t];
		indexInTeam[t] = 0;
	}

	for (int p = 0; p < m_numberOfPlayers; p++) {
		result.numbersOfParticipants[m_players[p].data.team]++;
	}

	result.participantIDs = new int32_t*[m_numberOfTeams];
	for (int t = 0; t < m_numberOfTeams; t++) {
		result.participantIDs[t] = new int32_t[result.numbersOfParticipants[t]];
		for (int p = 0; p < result.numbersOfParticipants[t]; p++) {
			result.participantIDs[t][p] = 0;
		}
	}	

	for (int p = 0; p < m_numberOfPlayers; p++) {
		// I probably over complicated this, this just puts the player IDs in the 2d array
		result.participantIDs[m_players[p].data.team][indexInTeam[m_players[p].data.team]] = m_players[p].data.id;
		indexInTeam[m_players[p].data.team]++;
	}	

	delete[] m_players, m_playerTeams;

	delete m_daggers, m_shockwaves;
}

void Logic::startRound()
{
	if (m_shockwaves->size() > 0) {
		m_shockwaves->clear();
	}
	if (m_daggers->size() > 0) {
		m_daggers->clear();
	}

	// Assume players in are ordered in the way they should be placed at the beginning of each round.
	for (int p = 0; p < m_numberOfPlayers; p++) {		
		m_players[p].data.pos[0] = (m_mapSize - MAP_PLAYER_BORDER) * (float) sin((double) p * (2 * PI / m_numberOfPlayers));
		m_players[p].data.pos[1] = (m_mapSize - MAP_PLAYER_BORDER) * (float) cos((double) p * (2 * PI / m_numberOfPlayers));

		m_players[p].isAlive = true;
		m_players[p].data.shieldDuration = 0;
		m_players[p].data.stoneDuration = 0;

		m_players[p].data.targetPos[0] = m_players[p].data.pos[0];
		m_players[p].data.targetPos[1] = m_players[p].data.pos[1];

		for (int c = 0; c < 5; c++) {
			m_players[p].data.cooldowns[c] = 0;
		}
	}
}

bool Logic::calculateCollision(float* pos1, float* mov1, float* pos2, float* mov2, float maxDist)
{
	float xcoeff = mov1[0] - mov2[0] - pos1[0] + pos2[0];
	float ycoeff = mov1[1] - mov2[1] - pos1[1] + pos2[1];

	float xconst = pos1[0] - pos2[0];
	float yconst = pos1[1] - pos2[1];

	float a = pow(xcoeff, 2) + pow(ycoeff, 2);
	float b = 2 * (xcoeff * xconst + ycoeff * yconst);
	float c = pow(xconst, 2) + pow(yconst, 2) - pow(maxDist, 2);

	float discriminant = pow(b, 2) - 4 * a * c;
	if (discriminant >= 0) {
		/* A collision occurs at some point but this could be in the past or future so check that it actually happened this tick
		0 <= t <= 1
		root1 <= 1 and root2 >= 0
		*/
		discriminant = sqrt(discriminant);

		float root1 = (-b - discriminant) / (2 * a);
		if (root1 <= 1.0f) {
			float root2 = (-b + discriminant) / (2 * a);
			if (root2 >= 0.0f) {
				return true;
			}
		}
	}

	return false;
}

int32_t Logic::getPlayerById(int32_t id)
{
	for (int p = 0; p < m_numberOfPlayers; p++) {
		if (m_players[p].data.id == id) {
			return p;
		}
	}
	return -1;
}
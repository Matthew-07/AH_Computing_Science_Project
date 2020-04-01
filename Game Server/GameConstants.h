#pragma once

// Player Inputs
#define INP_MOVE	0x01
#define	INP_SHIELD	0x02
#define INP_BLINK	0x03
#define INP_SHOCK	0x04
#define	INP_DAGGER	0x05
#define	INP_STONE	0x06

#define SHIELD_INDEX	0
#define BLINK_INDEX		1
#define SHOCK_INDEX		2
#define DAGGER_INDEX	3
#define STONE_INDEX		4

const int32_t	TICKS_PER_SECOND = 64;

const int32_t	MAX_SCORE = 5;

const float		PLAYER_SPEED = 60;
const float		PLAYER_SPEED_PER_TICK = PLAYER_SPEED / TICKS_PER_SECOND;
const int32_t	PLAYER_WIDTH = 24;

const float		SHOCKWAVE_SPEED = 600;
const float		SHOCKWAVE_SPEED_PER_TICK = SHOCKWAVE_SPEED / TICKS_PER_SECOND;
const int32_t	SHOCKWAVE_RANGE = 500;
const int32_t	SHOCKWAVE_COOLDOWN = 2.5 * TICKS_PER_SECOND;
// A shockwave takes range/speed time to finish and can at most be produced once every cooldown ticks.
const int32_t	MAX_SHOCKWAVES = (int32_t)ceil((double)SHOCKWAVE_RANGE / (double)SHOCKWAVE_SPEED / (double)SHOCKWAVE_COOLDOWN);
const int32_t	SHOCKWAVE_WIDTH = 5;
const int32_t	SHOCKWAVE_LINE_WIDTH = 3;
const int32_t	SHOCKWAVE_COLLISION_SIZE = PLAYER_WIDTH + SHOCKWAVE_WIDTH;

const float		DAGGER_SPEED = 450;
const float		DAGGER_SPEED_PER_TICK = DAGGER_SPEED / TICKS_PER_SECOND;
const int32_t	DAGGER_MAX_LIFETIME = 20 * TICKS_PER_SECOND; // Maximum time a dagger can exist for
const int32_t	DAGGER_COOLDOWN = 3 * TICKS_PER_SECOND;
const int32_t	MAX_DAGGERS = ceil(DAGGER_MAX_LIFETIME / DAGGER_COOLDOWN);
const int32_t	DAGGER_WIDTH = 8;
const int32_t	DAGGER_COLLISION_SIZE = PLAYER_WIDTH + DAGGER_WIDTH;

const int32_t	SHIELD_DURATION = round(0.2 * TICKS_PER_SECOND);
const int32_t	SHIELD_COOLDOWN = 2.5 * TICKS_PER_SECOND;

const int32_t	BLINK_COOLDOWN = 2 * TICKS_PER_SECOND;
const int32_t	BLINK_RANGE = 800;

const int32_t	STONE_DURATION = 300;
const int32_t	STONE_COOLDOWN = 600;

const int32_t	POST_BLINK_COOLDOWN = round(0.15 * TICKS_PER_SECOND);

const int32_t	MAP_SIZE_PER_PLAYER = 100;
const int32_t	MAP_SIZE_CONSTANT = 300;
const int32_t	MAP_PLAYER_BORDER = 100;
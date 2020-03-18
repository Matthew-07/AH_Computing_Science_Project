#pragma once

#include "BaseWindow.h"

class Graphics; class Network;

struct PlayerData; struct ShockwaveData; struct DaggerData;

class GameWindow :
	public BaseWindow<GameWindow>
{
public:
	GameWindow(Graphics* graphics, Network* nw);

	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	bool startGame();

private:
	PCWSTR ClassName() const;
	WCHAR m_className[MAX_LOADSTRING];

	RECT m_rect;

	Graphics* myGraphics = NULL;
	Network* network = NULL;

	void onPaint();
	void createGraphicResources();
	void discardGraphicResources();

	void extractGamestate(char* packet);

	int32_t m_userId;
	int32_t m_userTeam;

	ID2D1HwndRenderTarget* pRenderTarget;
	ID2D1SolidColorBrush* bBlack, * bWhite, * bPlayer, * bProjectileEnemy, * bProjectileAlly;
	ID2D1SolidColorBrush* bStone, * bShield;
	ID2D1SolidColorBrush* bOnCooldown, * bOffCooldown;
	std::vector<ID2D1SolidColorBrush*> teamBrushes;
	IDWriteTextFormat* pScoreTextFormat, * pCooldownTextFormat, *pFpsTextFormat;

	int32_t latestPacketNumber;
	std::chrono::steady_clock::time_point packetTimer;
	std::chrono::steady_clock::time_point frameRateTimer;
	int32_t frameCounter = 0;
	int32_t frameRate = 0;

	float m_camX, m_camY;

	int32_t m_mapSize;
	int32_t m_numberOfPlayers;
	int32_t m_maxNumberOfPlayers;
	int32_t m_numberOfTeams;

	int32_t* m_playerIds;
	int32_t* m_playerTeams;

	int32_t* m_teamScores;

	PlayerData* m_players;

	std::list<ShockwaveData> m_shockwaves;
	std::list<DaggerData> m_daggers;

	bool packetRecieved = false;
};


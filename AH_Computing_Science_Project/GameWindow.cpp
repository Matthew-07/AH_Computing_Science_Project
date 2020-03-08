#include "GameWindow.h"
#include "Graphics.h"
#include "Network.h"

#include "../Game Server/Logic.h"

PCWSTR GameWindow::ClassName() const
{
	return (PCWSTR)m_className;
}

GameWindow::GameWindow(Graphics* graphics, Network* nw)
{
	LoadStringW(m_inst, IDS_MAINWINDOWNAME, m_className, MAX_LOADSTRING);

	myGraphics = graphics;
	network = nw;
	m_rect = RECT();

	myGraphics->getWriteFactory()->CreateTextFormat(
		L"Ariel",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		48.0f,
		L"en-uk",
		&pScoreTextFormat
	);
	pScoreTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

	myGraphics->getWriteFactory()->CreateTextFormat(
		L"Ariel",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		24.0f,
		L"en-uk",
		&pCooldownTextFormat
	);
	pCooldownTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
}

LRESULT GameWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Input i;

	switch (uMsg) {
	case WM_CREATE:
	{
		latestPacketNumber = -1;
		break;
	}
	case WM_PAINT:
	{
		onPaint();
		break;
	}
	case WM_SIZE:
	{
		GetClientRect(m_hwnd, &m_rect);
		discardGraphicResources();
		InvalidateRect(m_hwnd, NULL, TRUE);
		break;
	}
	case CA_SHOWGAME:
	{
		if (wParam == SW_SHOW) {
			if (lParam != NULL) {
				m_userId = *(int32_t*)lParam;
			}
		}
		startGame();
		break;
	}
	case WM_COMMAND:
	{		
		switch (LOWORD(wParam)) {			
		case FUNC_SHIELD:
		{
			i.type = INP_SHIELD;
			i.playerId = m_userId;
			network->sendInput(&i);
			break;
		}

		case FUNC_STONE:
		{
			i.type = INP_STONE;
			i.playerId = m_userId;
			network->sendInput(&i);
			break;
		}

		case FUNC_BLINK:
		{
			i.type = INP_BLINK;
			i.playerId = m_userId;

			POINT p;
			GetCursorPos(&p);
			ScreenToClient(m_hwnd, &p);
			i.data.f[0] = p.x + m_camX;
			i.data.f[1] = p.y + m_camY;
			network->sendInput(&i);
			break;
		}

		case FUNC_SHOCK:
		{
			i.type = INP_SHOCK;
			i.playerId = m_userId;

			POINT p;
			GetCursorPos(&p);
			ScreenToClient(m_hwnd, &p);
			i.data.f[0] = p.x + m_camX;
			i.data.f[1] = p.y + m_camY;
			network->sendInput(&i);
			break;
		}
		
		case FUNC_DAGGER:
		{
			float minDist = -1.0f;
			int32_t minDistId = -1;

			POINT p;
			GetCursorPos(&p);
			ScreenToClient(m_hwnd, &p);

			int32_t userIndex = -1;
			for (int player = 0; player < m_numberOfPlayers; player++) {
				if (m_players[player].id == m_userId) {
					userIndex = player;
					break;
				}
			}

			for (int player = 0; player < m_numberOfPlayers; player++) {
				if (m_players[player].team != m_players[userIndex].team) {
					float dist = sqrt(pow(m_players[player].pos[0] - m_players[userIndex].pos[1], 2)
						+ pow(m_players[player].pos[0] - m_players[userIndex].pos[1], 2));

					if (dist < minDist || minDist < 0) {
						minDist = dist;
						minDistId = m_players[player].id;
					}

				}
			}

			i.type = INP_DAGGER;
			i.playerId = m_userId;
			i.data.i[0] = minDistId;
			network->sendInput(&i);
			break;
		}
		}
		break;
	}
	case WM_RBUTTONDOWN:
	{
		float xPos = GET_X_LPARAM(lParam) + m_camX;
		float yPos = GET_Y_LPARAM(lParam) + m_camY;

		i.type = INP_MOVE;
		i.playerId = m_userId;
		i.data.f[0] = xPos;
		i.data.f[1] = yPos;
		network->sendInput(&i);
		break;
	}
	}
	
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

bool GameWindow::startGame()
{
	int32_t maxGamestateSize;

	if (!network->getGameInfo(&m_maxNumberOfPlayers, &m_numberOfTeams, &m_playerIds, &m_playerTeams, &maxGamestateSize)) {

		int res = WSAGetLastError();
		char buff[64];
		_itoa_s(res, buff, 10);
		OutputDebugStringA("Error: ");
		OutputDebugStringA(buff);
		OutputDebugStringA("\n");

		MessageBoxA(NULL, "Unable to retrieve game information.", "Error", NULL);
		return false;
	}

	for (int p = 0; p < m_numberOfTeams; p++) {
		if (m_playerIds[p] == m_userId) {
			m_userTeam = m_playerTeams[p];
		}
	}

	m_mapSize = MAP_SIZE_PER_PLAYER * m_maxNumberOfPlayers + MAP_SIZE_CONSTANT;

	char *buff = new char[maxGamestateSize];
	m_players = new PlayerData[m_maxNumberOfPlayers];

	HACCEL hAccelTable = LoadAccelerators(m_inst, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	while (true)
	{
		MSG msg;
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT ||
				msg.message == WM_DESTROY ||
				msg.message == WM_CLOSE)
			{
				PostQuitMessage(0);
				break;
			}

			//if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			if (!TranslateAccelerator(m_hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else {
			if (network->recievePacket(buff)) {				
				packetTimer = std::chrono::steady_clock::now();				

				extractGamestate(buff);
			}
			InvalidateRect(m_hwnd, NULL, false);
		}
	}
	delete[] buff, m_players;
	packetRecieved = false;
	return false;
}

void GameWindow::onPaint() {
	createGraphicResources();

	// Begin Draw
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hwnd, &ps);

	pRenderTarget->BeginDraw();

	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF(0.8f, 0.8f, 0.8f)));	

	// Don't draw the game until the first packet has been recieved.
	if (packetRecieved) {

		// Draw Window
		float timeSinceLastPacket = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - packetTimer).count() / 1000000;

		// Only extrapolate for up to 500ms, then just draw the same frame until more data arrives
		if (timeSinceLastPacket > 0.5) {
			timeSinceLastPacket = 0.5;
		}

		// Center camera on player while player is alive
		for (int p = 0; p < m_numberOfPlayers; p++) {
			if (m_players[p].id == m_userId) {
				m_camX = m_players[p].pos[0] - m_rect.right / 2;
				m_camY = m_players[p].pos[1] - m_rect.bottom / 2;
				break;
			}
		}

		pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(-m_camX, -m_camY), m_mapSize,m_mapSize),bWhite);

		for (int p = 0; p < m_numberOfPlayers; p++) {

			float newPos[2];
			newPos[0] = m_players[p].pos[0];
			newPos[1] = m_players[p].pos[1];
			calculateMovement(newPos, m_players[p].targetPos, timeSinceLastPacket * PLAYER_SPEED);

			if (m_players[p].id == m_userId) {
				if (m_players[p].stoneDuration > 0) {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), 28, 28), bStone);
				}
				else {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), 28, 28), bPlayer);

					if (m_players[p].shieldDuration > 0) {
						pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), 26, 26), bShield, 5);
					}
				}				
			}
			else {				
				if (m_players[p].stoneDuration > 0) {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), 28, 28), bStone);
				}
				else {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), 28, 28), teamBrushes[m_players[p].team]);

					if (m_players[p].shieldDuration > 0) {
						pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), 26, 26), bShield, 5);
					}
				}
			}
		}

		for (auto shockwave : m_shockwaves) {

			float newPos[2];
			newPos[0] = shockwave.pos[0];
			newPos[1] = shockwave.pos[1];
			calculateMovement(newPos, shockwave.dest, timeSinceLastPacket * SHOCKWAVE_SPEED);

			if (shockwave.team == m_userTeam) {
				pRenderTarget->DrawLine(D2D1::Point2F(shockwave.start[0] - m_camX, shockwave.start[1] - m_camY),D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY),bProjectileAlly, 3);
				pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), 5, 5), bProjectileAlly);
			}
			else {
				pRenderTarget->DrawLine(D2D1::Point2F(shockwave.start[0] - m_camX, shockwave.start[1] - m_camY), D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), bProjectileEnemy, 3);
				pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), 5, 5), bProjectileEnemy);
			}
		}

		for (auto dagger : m_daggers) {

			int32_t index = -1;

			for (int p = 0; p < m_numberOfPlayers; p++) {
				if (m_players[p].id == dagger.targetId) {
					index = p;
					break;
				}
			}
			if (index >= 0) {

				float newPos[2];
				newPos[0] = dagger.pos[0];
				newPos[1] = dagger.pos[1];
				calculateMovement(newPos, m_players[index].pos, timeSinceLastPacket * DAGGER_SPEED);

				if (dagger.team == m_userTeam) {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), 8, 8), bProjectileAlly);
				}
				else {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), 8, 8), bProjectileEnemy);
				}

			}
		}
	}

	//End Draw
	pRenderTarget->EndDraw();

	EndPaint(m_hwnd, &ps);
}

void GameWindow::createGraphicResources()
{
	HRESULT hr = S_OK;
	if (pRenderTarget == NULL)
	{
		D2D1_SIZE_U size = D2D1::SizeU(m_rect.right, m_rect.bottom);

		hr = myGraphics->getFactory()->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, size),
			&pRenderTarget);

		pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &bBlack);
		pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &bWhite);

		pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green), &bPlayer);

		pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue), &bProjectileAlly);
		pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &bProjectileEnemy);

		pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray), &bStone);
		pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::MediumPurple), &bShield);

		UINT32 colours[5] = {
			D2D1::ColorF::Orange,
			D2D1::ColorF::Yellow,
			D2D1::ColorF::Pink,
			D2D1::ColorF::Purple,
			D2D1::ColorF::Brown
		};

		teamBrushes.clear();

		for (auto colour : colours) {
			ID2D1SolidColorBrush* bTeamBrush;
			pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(colour,1.0f), &bTeamBrush);
			teamBrushes.push_back(bTeamBrush);
		}
	}
}

void GameWindow::discardGraphicResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&bBlack);
	SafeRelease(&bWhite);
	SafeRelease(&bPlayer);
	SafeRelease(&bProjectileAlly);
	SafeRelease(&bProjectileEnemy);

	for (int i = 0; i < teamBrushes.size(); i++) {
		SafeRelease(&teamBrushes[i]);
	}
	teamBrushes.clear();
}

void GameWindow::extractGamestate(char* packet)
{
	//if (*(int32_t*)packet < latestPacketNumber) {
	//	return;		
	//}
	latestPacketNumber = *(int32_t*)packet;

	packet += 4;

	m_shockwaves.empty();
	m_daggers.empty();

	// Number of players
	if (*(int32_t*)packet > m_maxNumberOfPlayers || *(int32_t*)packet < 1) {
		// Something went wrong
		OutputDebugString(L"Too many players\n");
		return;
	}	
	m_numberOfPlayers = *(int32_t*)packet;

	//OutputDebugStringA(std::to_string(m_numberOfPlayers).c_str());

	packet += 4;

	for (int p = 0; p < m_numberOfPlayers; p++) {
		m_players[p] = *(PlayerData*) packet;
		packet += sizeof(PlayerData);
	}	

	// Shockwaves
	if (*(int32_t*)packet > m_numberOfPlayers * MAX_SHOCKWAVES || *(int32_t*)packet < 0) {
		// Something went wrong
		OutputDebugString(L"Too many shockwaves\n");
		return;
	}

	m_shockwaves.clear();

	int numberOfShockwaves = *(int32_t*)packet;
	packet += 4;

	for (int s = 0; s < numberOfShockwaves; s++) {
		ShockwaveData shockwave = *(ShockwaveData*)packet;
		packet += sizeof(ShockwaveData);
		m_shockwaves.push_back(shockwave);
	}

	// Daggers
	if (*(int32_t*)packet > m_numberOfPlayers* MAX_DAGGERS || *(int32_t*)packet < 0) {
		// Something went wrong
		OutputDebugString(L"Too many daggers\n");
		return;
	}

	m_daggers.clear();

	int numberOfdaggers = *(int32_t*)packet;
	packet += 4;

	for (int s = 0; s < numberOfdaggers; s++) {
		DaggerData dagger = *(DaggerData*)packet;
		packet += sizeof(DaggerData);
		m_daggers.push_back(dagger);
	}

	// At least one valid packet has been recieved!
	packetRecieved = true;
}

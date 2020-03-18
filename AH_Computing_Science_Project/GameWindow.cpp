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
		toPixels(24.0f),
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
		toPixels(14.0f),
		L"en-uk",
		&pCooldownTextFormat
	);
	pCooldownTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	pCooldownTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	myGraphics->getWriteFactory()->CreateTextFormat(
		L"Ariel",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		toPixels(18.0f),
		L"en-uk",
		&pFpsTextFormat
	);
}

LRESULT GameWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Input i;

	switch (uMsg) {
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
			startGame();
		}		
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

	m_teamScores = new int32_t[m_numberOfTeams];
	for (int t = 0; t < m_numberOfTeams; t++) {
		m_teamScores[t] = 0;
	}

	HACCEL hAccelTable = LoadAccelerators(m_inst, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	frameRateTimer = std::chrono::steady_clock::now();
	frameCounter = 0;
	frameRate = 0;
	
	network->clearUDPRecieveBuff();

	packetRecieved = false;
	latestPacketNumber = -1;

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
			if (network->checkForGameEnd()){
				ShowWindow(m_hwnd, SW_HIDE);				
				return true;
			}
			else if  (network->recievePacket(buff)) {	
				// Make sure the client recieves all waiting packets
				while (network->recievePacket(buff)) {}
				packetTimer = std::chrono::steady_clock::now();				

				extractGamestate(buff);
			}
			InvalidateRect(m_hwnd, NULL, false);
		}
	}

	m_daggers.clear();
	m_shockwaves.clear();

	delete[] buff, m_players, m_playerIds, m_playerTeams, m_teamScores;

	return false;
}

void GameWindow::onPaint() {
	createGraphicResources();

	// Begin Draw
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hwnd, &ps);

	pRenderTarget->BeginDraw();

	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF(0.8f, 0.8f, 0.8f)));	

	int32_t playerIndex = -1;

	// Don't draw the game until the first packet has been recieved.
	if (packetRecieved) {

		// Draw Window
		float timeSinceLastPacket = (float) std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - packetTimer).count() / 1000000.0f;

		// Only extrapolate for up to 500ms, then just draw the same frame until more data arrives
		if (timeSinceLastPacket > 0.5) {
			timeSinceLastPacket = 0.5;
		}

		// The players positions are extrapolated based on the time since the last frame.
		// The new positions of the players has to be store seperately to be used in calculating movement of the daggers
		float ** newPositions = new float*[m_numberOfPlayers];

		for (int p = 0; p < m_numberOfPlayers; p++) {

			newPositions[p] = new float[2];
			newPositions[p][0] = m_players[p].pos[0];
			newPositions[p][1] = m_players[p].pos[1];
			calculateMovement(newPositions[p], m_players[p].targetPos, timeSinceLastPacket * PLAYER_SPEED);
		}		

		// Center camera on player while player is alive
		for (int p = 0; p < m_numberOfPlayers; p++) {
			if (m_players[p].id == m_userId) {

				m_camX = newPositions[p][0] - m_rect.right / 2;
				m_camY = newPositions[p][1] - m_rect.bottom / 2;

				playerIndex = p;
				break;
			}
		}

		pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(-m_camX, -m_camY), m_mapSize, m_mapSize), bWhite);

		for (int p = 0; p < m_numberOfPlayers; p++) {

			if (m_players[p].id == m_userId) {
				if (m_players[p].stoneDuration > 0) {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPositions[p][0] - m_camX, newPositions[p][1] - m_camY), PLAYER_WIDTH, PLAYER_WIDTH), bStone);
				}
				else {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPositions[p][0] - m_camX, newPositions[p][1] - m_camY), PLAYER_WIDTH, PLAYER_WIDTH), bPlayer);

					if (m_players[p].shieldDuration > 0) {
						pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(newPositions[p][0] - m_camX, newPositions[p][1] - m_camY), PLAYER_WIDTH - 2, PLAYER_WIDTH - 2), bShield, 5);
					}
				}
			}
			else {
				if (m_players[p].stoneDuration > 0) {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPositions[p][0] - m_camX, newPositions[p][1] - m_camY), PLAYER_WIDTH, PLAYER_WIDTH), bStone);
				}
				else {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPositions[p][0] - m_camX, newPositions[p][1] - m_camY), PLAYER_WIDTH, PLAYER_WIDTH), teamBrushes[m_players[p].team]);

					if (m_players[p].shieldDuration > 0) {
						pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(newPositions[p][0] - m_camX, newPositions[p][1] - m_camY), PLAYER_WIDTH - 2, PLAYER_WIDTH - 2), bShield, 5);
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
				pRenderTarget->DrawLine(D2D1::Point2F(shockwave.start[0] - m_camX, shockwave.start[1] - m_camY), D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), bProjectileAlly, SHOCKWAVE_LINE_WIDTH);
				pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), SHOCKWAVE_WIDTH, SHOCKWAVE_WIDTH), bProjectileAlly);
			}
			else {
				pRenderTarget->DrawLine(D2D1::Point2F(shockwave.start[0] - m_camX, shockwave.start[1] - m_camY), D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), bProjectileEnemy, SHOCKWAVE_LINE_WIDTH);
				pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), SHOCKWAVE_WIDTH, SHOCKWAVE_WIDTH), bProjectileEnemy);
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
				calculateMovement(newPos, newPositions[index], timeSinceLastPacket * DAGGER_SPEED);

				if (dagger.team == m_userTeam) {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), DAGGER_WIDTH, DAGGER_WIDTH), bProjectileAlly);
				}
				else {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(newPos[0] - m_camX, newPos[1] - m_camY), DAGGER_WIDTH, DAGGER_WIDTH), bProjectileEnemy);
				}

			}
		}

		// Deallocate new postions 2d array
		for (int p = 0; p < m_numberOfPlayers; p++) {
			delete[] newPositions[p];
		}
		delete[] newPositions;
	}	

	frameCounter++;
	auto timeNow = std::chrono::steady_clock::now();
	int32_t timeElapsed = std::chrono::duration_cast<std::chrono::microseconds>(timeNow - frameRateTimer).count();
	if (timeElapsed >= 250000){
		frameRate = frameCounter * 1000000 / timeElapsed;
		frameRateTimer = timeNow;
		frameCounter = 0;
	}

	std::wstring fpsText = L"FPS: " + std::to_wstring(frameRate);
	pRenderTarget->DrawTextW(
		fpsText.c_str(),
		fpsText.length(),
		pFpsTextFormat,
		D2D1::RectF(toPixels(m_rect.right - toPixels(80.0f)), toPixels(4.0f) , m_rect.right - toPixels(4.0f), toPixels(4.0f)),
		bBlack
	);

	// Score - the first score displayed is that of the team the user is on
	std::wstring scoreString = std::to_wstring(m_teamScores[m_userTeam]);

	for (int t = 0; t < m_numberOfTeams; t++) {
		if (t == m_userTeam) continue;
		scoreString += L" : " + std::to_wstring(m_teamScores[t]);
	}

	pRenderTarget->DrawTextW(
		scoreString.c_str(),
		scoreString.length(),
		pScoreTextFormat,
		D2D1::RectF(toPixels(0.0f), toPixels(16.0f), m_rect.right, toPixels(16.0f)),
		bBlack
	);
	
	if (playerIndex >= 0) {
		std::wstring cooldownStrings[5];
		cooldownStrings[0] = L"(Q)\nShield\n";
		cooldownStrings[1] = L"(W)\nBlink\n";
		cooldownStrings[2] = L"(E)\nShockwave\n";
		cooldownStrings[3] = L"(S)\nDagger\n";
		cooldownStrings[4] = L"(D)\nStone Form\n";

		// Cooldowns
		for (int c = 0; c < 5; c++) {

			ID2D1SolidColorBrush* brush;
			if (m_players[playerIndex].cooldowns[c] > 0) {
				brush = bOnCooldown;
				// Rounds to 2 d.p. by multiplying by 100 (4.23536 -> 423.536), rounding to the nearest integer (423.536 -> 424) and diving by 100 (424 -> 4.24)
				float cooldown = round((double)m_players[playerIndex].cooldowns[c] / TICKS_PER_SECOND * 100) / 100;
				
				std::wstringstream stream;
				stream << std::fixed << std::setprecision(2) << cooldown;
				cooldownStrings[c] += stream.str();
			}
			else {
				brush = bOffCooldown;
			}

			pRenderTarget->FillRectangle(
				D2D1::RectF(
					m_rect.right / 2 + c * toPixels(88.0f) - toPixels(216.0f),
					m_rect.bottom - toPixels(112.0f),
					m_rect.right / 2 + c * toPixels(88.0f) - toPixels(136.0f),
					m_rect.bottom - toPixels(32.0f)),
				brush);

			pRenderTarget->DrawRectangle(
				D2D1::RectF(
					m_rect.right / 2 + c * toPixels(88.0f) - toPixels(216.0f),
					m_rect.bottom - toPixels(112.0f),
					m_rect.right / 2 + c * toPixels(88.0f) - toPixels(136.0f),
					m_rect.bottom - toPixels(32.0f)),
				bBlack);

			pRenderTarget->DrawTextW(
				cooldownStrings[c].c_str(),
				cooldownStrings[c].length(),
				pCooldownTextFormat,
				D2D1::RectF(
					m_rect.right / 2 + c * toPixels(88.0f) - toPixels(212.0f),
					m_rect.bottom - toPixels(112.0f),
					m_rect.right / 2 + c * toPixels(88.0f) - toPixels(140.0f),
					m_rect.bottom - toPixels(36.0f)
				),
				bBlack);
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

		pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.9f, 0.8f, 0.8f), &bOnCooldown);
		pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.9f, 0.9f, 0.9f), &bOffCooldown);

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
	if (*(int32_t*)packet < latestPacketNumber) {
		return;		
	}
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

	for (int t = 0; t < m_numberOfTeams; t++) {
		m_teamScores[t] = *(int32_t*)(packet);
		packet += 4;
	}

	// At least one valid packet has been recieved!
	packetRecieved = true;
}

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
	switch (uMsg) {
	case WM_CREATE:
	{
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
		startGame();
		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case FUNC_SHIELD:
			Input i;
			i.type = INP_SHIELD;
			i.playerId = userId;
			network->sendInput(&i);
			break;
		}
	}
	case WM_MBUTTONDOWN:
	{
		if (wParam == 0x0002) {
			float xPos = GET_X_LPARAM(lParam);
			float yPos = GET_Y_LPARAM(lParam);

			Input i;
			i.type = INP_MOVE;
			i.playerId = userId;
			i.data[0] = xPos;
			i.data[1] = yPos;
			network->sendInput(&i);
			break;
		}
	}
	}
	
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

bool GameWindow::startGame()
{
	int32_t maxGamestateSize;

	if (!network->getGameInfo(&m_numberOfPlayers, &m_numberOfTeams, &m_playerIds, &m_playerTeams, &maxGamestateSize)) {

		int res = WSAGetLastError();
		char buff[64];
		_itoa_s(res, buff, 10);
		OutputDebugStringA("Error: ");
		OutputDebugStringA(buff);
		OutputDebugStringA("\n");

		MessageBoxA(NULL, "Unable to retrieve game information.", "Error", NULL);
		return false;
	}

	char *buff = new char[maxGamestateSize];

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

			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else {
			if (network->recievePacket(buff)) {				
				packetTimer = std::chrono::steady_clock::now();
				packetRecieved = true;

				extractGamestate(buff);
			}
			InvalidateRect(m_hwnd, NULL, false);
		}
	}
	delete[] buff;
	packetRecieved = false;
	return false;
}

void GameWindow::onPaint() {
	createGraphicResources();

	// Begin Draw
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hwnd, &ps);

	pRenderTarget->BeginDraw();

	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF(1.0f, 1.0f, 1.0f)));

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
			if (m_players[p].id = userId) {
				m_camX = m_players[p].pos[0] - m_rect.right / 2;
				m_camX = m_players[p].pos[1] - m_rect.bottom / 2;
			}
		}

		for (int p = 0; p < m_numberOfPlayers; p++) {

			float newPos[2];
			newPos[0] = m_players[p].pos[0];
			newPos[1] = m_players[p].pos[1];
			calculateMovement(newPos, m_players[p].targetPos, timeSinceLastPacket * PLAYER_SPEED);

			if (m_players[p].id == userId) {
				pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(m_players[p].pos[0] - m_camX, m_players[p].pos[1] - m_camY), 30, 30), bPlayer);
			}
			else {
				pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(m_players[p].pos[0] - m_camX, m_players[p].pos[1] - m_camY), 28, 28), teamBrushes[m_players[p].team]);
			}
		}

		for (auto shockwave : m_shockwaves) {

			float newPos[2];
			newPos[0] = shockwave.pos[0];
			newPos[1] = shockwave.pos[1];
			calculateMovement(newPos, shockwave.dest, timeSinceLastPacket * SHOCKWAVE_SPEED);

			if (shockwave.team = userTeam) {
				//pRenderTarget->DrawLine();
			}
			else {
				//pRenderTarget->DrawLine();
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

				if (dagger.team = userTeam) {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(dagger.pos[0] - m_camX, dagger.pos[1] - m_camY), 8, 8), bProjectileAlly);
				}
				else {
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(dagger.pos[0] - m_camX, dagger.pos[1] - m_camY), 8, 8), bProjectileEnemy);
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

		D2D1_COLOR_F colors[5] = {
			D2D1::ColorF::Orange,
			D2D1::ColorF::Yellow,
			D2D1::ColorF::Pink,
			D2D1::ColorF::Purple,
			D2D1::ColorF::Brown
		};
	}
}

void GameWindow::discardGraphicResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&bBlack);
}

void GameWindow::extractGamestate(char* packet)
{
	latestPacketNumber = *(int32_t*)packet;
	packet += 4;

	if (m_players != nullptr) {
		delete[] m_players;
	}
	m_shockwaves.empty();
	m_daggers.empty();

	// Number of players
	int32_t m_numberOfPlayers = *(int32_t*) packet;
	packet += 4;

	m_players = new PlayerData[m_numberOfPlayers];

	for (int p = 0; p < m_numberOfPlayers; p++) {
		m_players[p] = *(PlayerData*) packet;
		packet += sizeof(PlayerData);
	}

	m_shockwaves.clear();
	
	int numberOfShockwaves = *(int32_t*)packet;
	packet += 4;

	for (int s = 0; s < numberOfShockwaves; s++) {
		ShockwaveData shockwave = *(ShockwaveData*)packet;
		packet += sizeof(ShockwaveData);
		m_shockwaves.push_back(shockwave);
	}

	m_shockwaves.clear();

	int numberOfdaggers = *(int32_t*)packet;
	packet += 4;

	for (int s = 0; s < numberOfdaggers; s++) {
		DaggerData dagger = *(DaggerData*)packet;
		packet += sizeof(DaggerData);
		m_daggers.push_back(dagger);
	}
}

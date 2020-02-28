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
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void GameWindow::onPaint() {
	createGraphicResources();

	// Begin Draw
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hwnd, &ps);

	pRenderTarget->BeginDraw();

	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF(1.0f, 1.0f, 1.0f)));

	// Draw Window
	int32_t timeSinceLastPacket = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - packetTimer).count();
	
	// Only extrapolate for up to 500ms, then just draw the same frame until more data arrives
	if (timeSinceLastPacket > 500000) {
		timeSinceLastPacket = 500000;
	}

	for (int p = 0; p < m_numberOfPlayers; p++) {

		float newPos[2];
		newPos[0] = m_players[p].pos[0];
		newPos[1] = m_players[p].pos[1];
		calculateMovement(newPos, m_players[p].targetPos, PLAYER_SPEED);

		if (m_players[p].id == userId) {
			pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(m_players[p].pos[0], m_players[p].pos[1]), 30, 30), bPlayer);
		}
		else {
			pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(m_players[p].pos[0], m_players[p].pos[1]), 28, 28), teamBrushes[m_players[p].team]);
		}
	}

	for (auto shockwave : m_shockwaves) {		

		float newPos[2];
		newPos[0] = shockwave.pos[0];
		newPos[1] = shockwave.pos[1];
		calculateMovement(newPos, shockwave.dest, PLAYER_SPEED);

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
			calculateMovement(newPos, m_players[index].pos, PLAYER_SPEED);

			if (dagger.team = userTeam) {
				pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(dagger.pos[0], dagger.pos[1]), 8, 8), bProjectileAlly);
			}
			else {
				pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(dagger.pos[0], dagger.pos[1]), 8, 8), bProjectileEnemy);
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

#include "MainWindow.h"
#include "Graphics.h"

PCWSTR MainWindow::ClassName() const
{
	return (PCWSTR) m_className;
}

MainWindow::MainWindow(Graphics * graphics) : pRenderTarget(NULL)
{
	LoadStringW(m_inst, IDS_MAINWINDOWNAME, m_className, MAX_LOADSTRING);

	myGraphics = graphics;
	m_rect = RECT();
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		break;
	case WM_SIZE:
		GetClientRect(m_hwnd, &m_rect);
		discardGraphicResources();
		InvalidateRect(m_hwnd, NULL, TRUE);		
		break;
	case WM_PAINT:
		onPaint();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void MainWindow::onPaint() {
	createGraphicResources();

	// Begin Draw
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hwnd, &ps);

	pRenderTarget->BeginDraw();	

	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF(0.95f,0.95f,0.98f)));

	// Draw Window



	//End Draw
	pRenderTarget->EndDraw();

	EndPaint(m_hwnd, &ps);
}

void MainWindow::createGraphicResources()
{
	HRESULT hr = S_OK;
	if (pRenderTarget == NULL)
	{
		D2D1_SIZE_U size = D2D1::SizeU(m_rect.right, m_rect.bottom);

		hr = myGraphics->getFactory()->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, size),
			&pRenderTarget);

		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightGray), &pBackgroundBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBombBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.2f), &pExplosionBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green), &pFoodBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &pBorderBrush);
		//pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.5f, 1.0f, 0.5f, 0.15f), &pSightRangeBrush);
	}
}

void MainWindow::discardGraphicResources()
{
	SafeRelease(&pRenderTarget);
}

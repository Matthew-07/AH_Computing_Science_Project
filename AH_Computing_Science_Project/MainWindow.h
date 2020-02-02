#pragma once

#include "BaseWindow.h"
class MainWindow :
	public BaseWindow<MainWindow>
{
public:
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);	
private:
	PCWSTR ClassName() const;
};


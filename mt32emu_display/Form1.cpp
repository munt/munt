#include "stdafx.h"
#include "Form1.h"
#include <windows.h>

using namespace mt32emu_display;

[STAThreadAttribute]
int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	Application::Run(new Form1());
	return 0;
}

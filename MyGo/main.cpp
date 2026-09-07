// main.cpp
#include "../UI/GoUI.h"
#include <windows.h>

int main() {
	GoUI ui;
	ui.run();
	return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	return main();
}
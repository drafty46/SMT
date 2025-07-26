#include "shared.h"
#include "gui.h"
#include "config.h"
#include "input.h"
#include "memory.h"

HMODULE g_hModule = NULL;
std::atomic<bool> hasConsole = false;
std::atomic<bool> alive = true;

void AttachConsole()
{
	if (!hasConsole) {
		AllocConsole();
		hasConsole = true;
		freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
		freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
		freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
		SetConsoleTitleA("Logging Console");

		std::cout.clear();
		std::cerr.clear();
		std::clog.clear();

		LogMessage("Started Logging");
	}
}

void DetachConsole() {
	if (hasConsole) {
		LogMessage("Console Detached. You can close this window.");
		FreeConsole();
		hasConsole = false;
	}
}

void DetachDLL() {
	CreateThread(nullptr, 0, [](LPVOID lpParam) -> DWORD {
		HMODULE hMod = (HMODULE)lpParam;
		FreeLibraryAndExitThread(hMod, 0);
		return 0;
		}, g_hModule, 0, nullptr);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	//AttachConsole();
	LoadIniConfig();
	InitMemory();
	//Sleep(10000);
	InitGui();
	while (!isGuiInitialized) { Sleep(100); }
	InitInput();
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_hModule = hMod;
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		//SaveIniConfig();
		ShutdownInput();
		do { Sleep(100); } while (keepAliveInput);
		ShutdownMemory();
		ShutdownGui();
		DetachConsole();
		break;
	}
	return TRUE;
}
#pragma once
#include "shared.h"

typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;

extern Present originalPresent;
extern HRESULT __stdcall hookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
extern std::atomic<bool> showGui;
extern void InitGui();
extern void ShutdownGui();
extern HWND window;
extern std::atomic<bool> isGuiInitialized;
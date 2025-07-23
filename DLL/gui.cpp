#include "shared.h"
#include "gui.h"
#include "config.h"
#include "memory.h"

Present originalPresent;
HWND window = NULL;
WNDPROC originalWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;
std::atomic<bool> showGui = false;
POINT topLeft;
int32_t width;
int32_t height;
float padding;
float tableWidth;

extern std::atomic<bool> alive;
extern std::atomic<bool> hasConsole;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern void AttachConsole();
extern void DetachConsole();
extern void DetachDLL();

void InitGui() {
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)&originalPresent, hookedPresent);
			init_hook = true;
		}
	} while (!init_hook);
}

void ShutdownGui() {
	alive = false;
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	if (pContext) {
		pContext->Release();
		pContext = nullptr;
	}
	if (pDevice) {
		pDevice->Release();
		pDevice = nullptr;
	}
	if (mainRenderTargetView) {
		mainRenderTargetView->Release();
		mainRenderTargetView = nullptr;
	}
	if (window && originalWndProc) {
		SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)originalWndProc);
		originalWndProc = nullptr;
	}
	kiero::shutdown();
}

void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	io.ConfigDebugHighlightIdConflicts = false;
	RECT rect;
	GetClientRect(window, &rect);
	topLeft = { rect.left, rect.top };
	ClientToScreen(window, &topLeft);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;
	tableWidth = width * 0.9f * 0.33f;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
	if (iniConfig["KEYBOARD"]["SHOW MENU"].as<std::string>() == "NONE") {
		showGui = true;
	}
}

LRESULT __stdcall hookedWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (!alive) {
		return CallWindowProc(originalWndProc, hWnd, uMsg, wParam, lParam);
	}

	if (GetForegroundWindow() == window && showGui) {
		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
	}

	if (showGui && (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)) {
		return TRUE;
	}

	return CallWindowProc(originalWndProc, hWnd, uMsg, wParam, lParam);
}

std::atomic<bool> isGuiInitialized = false;
HRESULT __stdcall hookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!alive) {
		return originalPresent(pSwapChain, SyncInterval, Flags);
	}
	if (!isGuiInitialized)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			originalWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)hookedWndProc);
			InitImGui();
			isGuiInitialized = true;
		}

		else
			return originalPresent(pSwapChain, SyncInterval, Flags);
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (!foundOffsets) {
		ImFont* font = ImGui::GetFont();
		float fontSize = 24.0f;
		ImDrawList* drawList = ImGui::GetForegroundDrawList();
		drawList->AddText(font, fontSize, ImVec2(10, 10), IM_COL32(255, 0, 0, 255), "Searching offsets...");
	}

	if (showGui) {
		int32_t count = 0;
		ImGui::Begin("SnowRunner Manual Transmission", nullptr,
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoMove
		);
		ImGui::SetWindowPos(ImVec2(topLeft.x + width * 0.05f, topLeft.y + height * 0.05f), ImGuiCond_Always);
		ImGui::SetWindowSize(ImVec2(width * 0.9f, height * 0.6f), ImGuiCond_Always);

		ImGui::BeginChild("KeyboardTable", ImVec2(tableWidth, height * 0.545f), true);
		if (ImGui::BeginTable("Settings##1", 2)) {
			ImGui::TableSetupColumn("Keyboard", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();

			for (auto entry : iniConfig["KEYBOARD"]) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Dummy(ImVec2(0, 0.1f));
				ImGui::Text(entry.first.c_str());
				ImGui::TableSetColumnIndex(1);
				std::string buttonText = entry.second.as<std::string>() + "##" + std::to_string(++count);
				if (ImGui::Button(buttonText.c_str())) {
					iniConfig["KEYBOARD"][entry.first] = "LISTENING";
				}
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
					if (entry.second.as<std::string>() == "LISTENING") {
						iniConfig["KEYBOARD"][entry.first] = "FOUND";
					}
					else {
						iniConfig["KEYBOARD"][entry.first] = "NONE";
					}
				}
			}
			ImGui::EndTable();
		}
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("ControllerTable", ImVec2(tableWidth, height * 0.545f), true);
		if (ImGui::BeginTable("Settings##2", 2)) {
			ImGui::TableSetupColumn("Controller", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();

			for (auto entry : iniConfig["CONTROLLER"]) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Dummy(ImVec2(0, 0.1f));
				ImGui::Text(entry.first.c_str());
				ImGui::TableSetColumnIndex(1);
				std::string buttonText = entry.second.as<std::string>() + "##" + std::to_string(++count);
				if (ImGui::Button(buttonText.c_str())) {
					iniConfig["CONTROLLER"][entry.first] = "LISTENING";
				}
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
					if (entry.second.as<std::string>() == "LISTENING") {
						iniConfig["CONTROLLER"][entry.first] = "FOUND";
					}
					else {
						iniConfig["CONTROLLER"][entry.first] = "NONE";
					}
				}
			}
			ImGui::EndTable();
		}
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("OptionsTable", ImVec2(tableWidth, height * 0.545f), true);
		if (ImGui::BeginTable("Settings##3", 2)) {
			ImGui::TableSetupColumn("Options", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();

			for (auto entry : iniConfig["OPTIONS"]) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Dummy(ImVec2(0, 0.1f));
				ImGui::Text(entry.first.c_str());
				ImGui::TableSetColumnIndex(1);
				if (ImGui::Button(entry.second.as<bool>() ? std::string("True##" + entry.first).c_str() : std::string("False##" + entry.first).c_str())) {
					iniConfig["OPTIONS"][entry.first] = !entry.second.as<bool>();
				}
			}

			ImGui::EndTable();
		}
		ImGui::EndChild();

		if (ImGui::Button("Save config")) {
			SaveIniConfig();
		}
		ImGui::SameLine();
		ImGui::Text("Use left click to change keybind. Use right click to confirm or to clear if already set.");
		ImGui::SameLine();
		ImGui::InvisibleButton("##debug_separator", ImVec2(width * 0.43f, ImGui::GetItemRectSize().y));
		ImGui::SameLine();
		if (hasConsole) {
			if (ImGui::Button("Detach console")) {
				DetachConsole();
			}
		}
		else {
			if (ImGui::Button("Attach console")) {
				AttachConsole();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Unload")) {
			DetachDLL();
			//MessageBoxA(window, "You thought", "SIKE!", MB_ICONERROR | MB_OK);
		}
		ImGui::End();
	}
	ImGui::Render();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return originalPresent(pSwapChain, SyncInterval, Flags);
}
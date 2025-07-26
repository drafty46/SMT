#include "shared.h"
#include "gui.h"
#include "config.h"
#include "memory.h"
#include "input.h"
#include "ttlakes_font.h"
#include "blackbox.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
ID3D11ShaderResourceView* boxTexture = NULL;
int32_t boxWidth, boxHeight, boxChannels;

extern std::atomic<bool> alive;
extern std::atomic<bool> hasConsole;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern void AttachConsole();
extern void DetachConsole();
extern void DetachDLL();

ID3D11ShaderResourceView* CreateTextureFromPixels(unsigned char* pixels, int width, int height, ID3D11Device* device) {
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pixels;
	initData.SysMemPitch = width * 4;

	ID3D11Texture2D* texture = nullptr;
	HRESULT hr = device->CreateTexture2D(&desc, &initData, &texture);
	if (FAILED(hr)) return nullptr;

	ID3D11ShaderResourceView* srv = nullptr;
	hr = device->CreateShaderResourceView(texture, nullptr, &srv);
	texture->Release(); // SRV holds a reference
	return srv;
}



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
	//io.ConfigDebugHighlightIdConflicts = false;
	io.FontDefault = io.Fonts->AddFontFromMemoryTTF((void*)TTLakesNeue_DemiBold_ttf, TTLakesNeue_DemiBold_ttf_len, 24.0f);
	RECT rect;
	GetClientRect(window, &rect);
	topLeft = { rect.left, rect.top };
	ClientToScreen(window, &topLeft);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;
	tableWidth = width * 0.9f / 5;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
	if (iniConfig["KEYBOARD"]["SHOW MENU"].as<std::string>() == "NONE") {
		showGui = true;
	}
	unsigned char* pixels = stbi_load_from_memory(blackBox_png, blackBox_png_len, &boxWidth, &boxHeight, &boxChannels, 4);
	boxTexture = CreateTextureFromPixels(pixels, boxWidth, boxHeight, pDevice);
	stbi_image_free(pixels);

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
		float fontSize = ImGui::GetFontSize();
		ImDrawList* drawList = ImGui::GetForegroundDrawList();
		drawList->AddText(font, fontSize, ImVec2(10, 10), IM_COL32(255, 0, 0, 255), "SEARCHING OFFSETS...");
	}

	if (iniConfig["KEYBOARD"]["RANGE HIGH"].as<std::string>() != "NONE" || iniConfig["KEYBOARD"]["RANGE LOW"].as<std::string>() != "NONE" ||
		iniConfig["CONTROLLER"]["RANGE HIGH"].as<std::string>() != "NONE" || iniConfig["CONTROLLER"]["RANGE LOW"].as<std::string>() != "NONE") {
		ImFont* font = ImGui::GetFont();
		float fontSize = 30.0f;
		ImDrawList* drawList = ImGui::GetForegroundDrawList();
		ImVec2 pos = ImVec2(width * 0.88f, height * 0.975f);
		ImVec2 boxPos = ImVec2(pos.x * 0.975f, pos.y * 0.999f);
		drawList->AddImage((ImTextureID)boxTexture, boxPos, ImVec2(boxPos.x + boxWidth, boxPos.y + boxHeight * 0.8f), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 127));
		switch (range) {
		case -1: {
			drawList->AddText(font, fontSize, ImVec2(pos.x + 1, pos.y + 1), IM_COL32(0, 0, 0, 192), "RANGE: LOW");
			drawList->AddText(font, fontSize, pos, IM_COL32(255, 255, 255, 255), "RANGE: LOW");
			break;
		}
		case 0: {
			drawList->AddText(font, fontSize, ImVec2(pos.x + 1, pos.y + 1), IM_COL32(0, 0, 0, 192), "RANGE: NORMAL");
			drawList->AddText(font, fontSize, pos, IM_COL32(255, 255, 255, 255), "RANGE: NORMAL");
			break;
		}
		case 1: {
			drawList->AddText(font, fontSize, ImVec2(pos.x + 1, pos.y + 1), IM_COL32(0, 0, 0, 192), "RANGE: HIGH");
			drawList->AddText(font, fontSize, pos, IM_COL32(255, 255, 255, 255), "RANGE: HIGH");
			break;
		}
		}
	}

	if (showGui) {
		int32_t count = 0;
		ImGui::Begin("SnowRunner Manual Transmission v" STR(VERSION), nullptr,
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoMove
		);
		ImGui::SetWindowPos(ImVec2(topLeft.x + width * 0.05f, topLeft.y + height * 0.05f), ImGuiCond_Always);
		ImGui::SetWindowSize(ImVec2(width * 0.9f, height * 0.65f), ImGuiCond_Always);

		ImGui::BeginChild("KeyboardTable", ImVec2(tableWidth * 2, height * 0.545f), true);
		if (ImGui::BeginTable("Settings##1", 2, ImGuiTableFlags_ScrollX)) {
			ImGui::TableSetupScrollFreeze(0, 1);
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
					tempPressed.clear();
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
		ImGui::BeginChild("ControllerTable", ImVec2(tableWidth * 2, height * 0.545f), true);
		if (ImGui::BeginTable("Settings##2", 2), ImGuiTableFlags_ScrollX) {
			ImGui::TableSetupScrollFreeze(0, 1);
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
					tempPressed.clear();
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
		ImGui::BeginChild("OptionsTable", ImVec2(tableWidth * 0.9f, height * 0.545f), true);
		if (ImGui::BeginTable("Settings##3", 2, ImGuiTableFlags_ScrollX)) {
			ImGui::TableSetupScrollFreeze(0, 1);
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
		//ImGui::SameLine();
		//ImGui::Text("Use left click to change keybind. Use right click to confirm or to clear if already set.");
		//ImGui::SameLine();
		//ImGui::InvisibleButton("##debug_separator", ImVec2(width * 0.43f, ImGui::GetItemRectSize().y));
		//ImGui::SameLine();
		//if (hasConsole) {
		//	if (ImGui::Button("Detach console")) {
		//		DetachConsole();
		//	}
		//}
		//else {
		//	if (ImGui::Button("Attach console")) {
		//		AttachConsole();
		//	}
		//}
		//ImGui::SameLine();
		//if (ImGui::Button("Unload")) {
		//	DetachDLL();
		//	//MessageBoxA(window, "You thought", "SIKE!", MB_ICONERROR | MB_OK);
		//}
		ImGui::End();
	}
	ImGui::Render();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return originalPresent(pSwapChain, SyncInterval, Flags);
}
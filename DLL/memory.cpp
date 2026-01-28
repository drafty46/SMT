#include "shared.h"
#include "memory.h"
#include "config.h"
#include "game_data.h"

HMODULE hModule = GetModuleHandleA(NULL);
MODULEINFO mInfo;
bool temp = GetModuleInformation(GetCurrentProcess(), hModule, &mInfo, sizeof(MODULEINFO));
size_t base = (size_t)mInfo.lpBaseOfDll;
size_t sizeOfImage = ((PIMAGE_NT_HEADERS)((uint8_t*)hModule + ((PIMAGE_DOS_HEADER)hModule)->e_lfanew))->OptionalHeader.SizeOfCode;

uint32_t PatternScan(const char* signature, size_t begin = 0, size_t end = 0)
{
	static auto pattern_to_byte = [](const char* pattern)
		{
			auto bytes = std::vector<char>{};
			auto start = const_cast<char*>(pattern);
			auto end = const_cast<char*>(pattern) + strlen(pattern);

			for (auto current = start; current < end; current++)
			{
				if (*current == '?')
				{
					current++;
					if (*current == '?')
						current++;
					bytes.push_back('\?');
				}
				else
				{
					bytes.push_back(strtoul(current, &current, 16));
				}
			}
			return bytes;
		};

	auto patternBytes = pattern_to_byte(signature);

	size_t patternLength = patternBytes.size();
	auto data = patternBytes.data();

	uint32_t result;
	size_t count = 0;

	if (!end) {
		end = sizeOfImage;
	}

	for (size_t i = begin; i < end - patternLength; i++)
	{
		bool found = true;
		for (size_t j = 0; j < patternLength; j++)
		{
			char a = '\?';
			char b = *(char*)(base + i + j);
			found &= data[j] == a || data[j] == b;
		}
		if (found)
		{
			result = i;
			count++;
		}
	}
	if (count == 1) {
		return result;
	}
	return NULL;
}

uint32_t ToLittleEndian(uint32_t value) {
	uint8_t b0 = (value >> 0) & 0xFF;
	uint8_t b1 = (value >> 8) & 0xFF;
	uint8_t b2 = (value >> 16) & 0xFF;
	uint8_t b3 = (value >> 24) & 0xFF;

	return ((uint32_t)b0 << 0) |
		((uint32_t)b1 << 8) |
		((uint32_t)b2 << 16) |
		((uint32_t)b3 << 24);
}

int32_t DigAHole(uintptr_t result) {
	result += base;
	uintptr_t address = result;
	for (; *(uint8_t*)(address) != 0xE8; address++) {}
	address++;
	int32_t value = ToLittleEndian(*(int32_t*)(address));
	address += 4;
	value = result - base + value + (address - result);
	address = base + value;
	for (; *(uint8_t*)(address) != 0x05; address++) {}
	address++;
	value = ToLittleEndian(*(int32_t*)(address));
	address += 4;
	value = address - base + value;
	return value;
}

combine_TRUCK_CONTROL** TruckControlPtr = nullptr;
Fnc_ShiftGear* ShiftGearO = nullptr;
Fnc_ShiftToAutoGear* ShiftToAutoGearO = nullptr;
Fnc_ShiftToHigh* ShiftToHighO = nullptr;
Fnc_ShiftToReverse* ShiftToReverseO = nullptr;
Fnc_ShiftToNeutral* ShiftToNeutralO = nullptr;
Fnc_GetMaxGear* GetMaxGearO = nullptr;
Fnc_DisableAutoAndShift* DisableAutoAndShiftO = nullptr;
Fnc_SetPowerCoef* SetPowerCoefO = nullptr;
Fnc_SetCurrentVehicle* SetCurrentVehicleO = nullptr;

Vehicle* GetCurrentVehicle() {
	combine_TRUCK_CONTROL* truckCtrl = *TruckControlPtr;
	if (truckCtrl == nullptr) {
		return nullptr;
	}
	return truckCtrl->CurVehicle;
}

void InitMemory() {
	int32_t ShiftGearOffset = PatternScan("48 89 74 24 10 48 89 7C 24 18 41 56 48 83 EC 20 48 8B 81 48 01 00 00 48 8B F1 48 B9 FF FF FF FF FF FF 00 FF 8B FA 48 23 C1 74");//
	LogMessage("ShiftGear:", std::format("{:08x}", ShiftGearOffset));
	int32_t ShiftToAutoGearOffset = PatternScan("40 57 48 81 EC 80 00 00 00 48 8B 41 68 48 8B F9 48 89 9C 24 90 00 00 00 0F 29 7C 24 60 C6 40 3C 01 48 8B 41 60 48 8B 90 30 02 00 00 0F 10 8A 30 02 00 00 0F 59 8A 70 01 00 00");//
	LogMessage("ShiftToAutoGear:", std::format("{:08x}", ShiftToAutoGearOffset));
	int32_t ShiftToHighOffset = PatternScan("40 53 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 8B CB 8D 50 01 48 83 C4 20 5B");//
	LogMessage("ShiftToHigh:", std::format("{:08x}", ShiftToHighOffset));
	int32_t ShiftToReverseOffset = PatternScan("BA FF FF FF FF E9 ? ? ? ? CC");//
	LogMessage("ShiftToReverse:", std::format("{:08x}", ShiftToReverseOffset));
	int32_t ShiftToNeutralOffset = PatternScan("33 D2 E9 ? ? ? ? CC", ShiftToHighOffset, ShiftToReverseOffset);
	LogMessage("ShiftToNeutral:", std::format("{:08x}", ShiftToNeutralOffset));
	int32_t GetMaxGearOffset = PatternScan("48 8B 41 68 48 8B 50 58 48 8B 48 60 48 3B D1 75 ? 33 C0 C3 48 2B CA 48 C1 F9 02 8D 41 FE");//
	LogMessage("GetMaxGear:", std::format("{:08x}", GetMaxGearOffset));
	int32_t DisableAutoAndShiftOffset = PatternScan("48 8B 41 68 C6 40 3C 00 E9");//
	LogMessage("DisableAutoAndShift:", std::format("{:08x}", DisableAutoAndShiftOffset));
	int32_t SetPowerCoefOffset = PatternScan("48 8B 41 68 F3 0F 11 48 38 C3");//
	LogMessage("SetPowerCoef:", std::format("{:08x}", SetPowerCoefOffset));
	int32_t SetCurrentVehicleOffset = PatternScan("48 8B C4 53 57 48 81 EC 98 00 00 00 48 8B FA 48 8B D9 48 39 51 08 0F 84 ? ? ? ? 48 89 68 E8 48 83 C1 70 48 89 70 E0 4C 89 70 D8 4C 89 78 D0");//
	LogMessage("SetCurrentVehicle:", std::format("{:08x}", SetCurrentVehicleOffset));
	int32_t combine_TRUCK_CONTROLOffset = DigAHole(PatternScan("40 53 48 83 EC 20 48 8B D9 E8 ? ? ? ? 33 C9 48 89 18"));//
	LogMessage("combine_TRUCK_CONTROL:", std::format("{:08x}", (combine_TRUCK_CONTROLOffset)));

	TruckControlPtr = (combine_TRUCK_CONTROL**)(base + combine_TRUCK_CONTROLOffset);
	ShiftGearO = (Fnc_ShiftGear*)(base + ShiftGearOffset);
	ShiftToAutoGearO = (Fnc_ShiftToAutoGear*)(base + ShiftToAutoGearOffset);
	ShiftToHighO = (Fnc_ShiftToHigh*)(base + ShiftToHighOffset);
	ShiftToReverseO = (Fnc_ShiftToReverse*)(base + ShiftToReverseOffset);
	ShiftToNeutralO = (Fnc_ShiftToNeutral*)(base + ShiftToNeutralOffset);
	GetMaxGearO = (Fnc_GetMaxGear*)(base + GetMaxGearOffset);
	DisableAutoAndShiftO = (Fnc_DisableAutoAndShift*)(base + DisableAutoAndShiftOffset);
	SetPowerCoefO = (Fnc_SetPowerCoef*)(base + SetPowerCoefOffset);
	SetCurrentVehicleO = (Fnc_SetCurrentVehicle*)(base + SetCurrentVehicleOffset);

	DetourRestoreAfterWith();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((PVOID*)&ShiftGearO, (PVOID)Hooked_ShiftGear);
	DetourAttach((PVOID*)&ShiftToAutoGearO, (PVOID)Hooked_ShiftToAutoGear);
	DetourAttach((PVOID*)&ShiftToHighO, (PVOID)Hooked_ShiftToHigh);
	DetourAttach((PVOID*)&ShiftToReverseO, (PVOID)Hooked_ShiftToReverse);
	DetourAttach((PVOID*)&ShiftToNeutralO, (PVOID)Hooked_ShiftToNeutral);
	DetourAttach((PVOID*)&GetMaxGearO, (PVOID)Hooked_GetMaxGear);
	DetourAttach((PVOID*)&DisableAutoAndShiftO, (PVOID)Hooked_DisableAutoAndShift);
	DetourAttach((PVOID*)&SetPowerCoefO, (PVOID)Hooked_SetPowerCoef);
	DetourAttach((PVOID*)&SetCurrentVehicleO, (PVOID)Hooked_SetCurrentVehicle);
	DetourTransactionCommit();

	if (Vehicle* veh = GetCurrentVehicle()) {
		IsInAuto[veh] = veh->TruckAction->IsInAutoMode;
		veh->TruckAction->IsInAutoMode = false;
		veh->ShiftToGear(1, 1.01);
	}

	LogMessage("init", base);
}

void ShutdownMemory() {
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach((PVOID*)&ShiftGearO, (PVOID)Hooked_ShiftGear);
	DetourDetach((PVOID*)&ShiftToAutoGearO, (PVOID)Hooked_ShiftToAutoGear);
	DetourDetach((PVOID*)&ShiftToHighO, (PVOID)Hooked_ShiftToHigh);
	DetourDetach((PVOID*)&ShiftToReverseO, (PVOID)Hooked_ShiftToReverse);
	DetourDetach((PVOID*)&ShiftToNeutralO, (PVOID)Hooked_ShiftToNeutral);
	DetourDetach((PVOID*)&GetMaxGearO, (PVOID)Hooked_GetMaxGear);
	DetourDetach((PVOID*)&DisableAutoAndShiftO, (PVOID)Hooked_DisableAutoAndShift);
	DetourDetach((PVOID*)&SetPowerCoefO, (PVOID)Hooked_SetPowerCoef);
	DetourDetach((PVOID*)&SetCurrentVehicleO, (PVOID)Hooked_SetCurrentVehicle);
	DetourTransactionCommit();
}
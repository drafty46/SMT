#include "shared.h"
#include "game_data.h"
#include "memory.h"
#include "config.h"
#include "input.h"

std::atomic<float> currentCoef = 1.01f;

std::unordered_map <Vehicle*, std::atomic<bool>> IsInAuto{};

void Vehicle::SetPowerCoef(float coef) { Hooked_SetPowerCoef(this, coef); }

std::int32_t Vehicle::GetMaxGear() const {
	return GetMaxGearO(this);
}

bool Vehicle::ShiftToGear(std::int32_t targetGear, float powerCoef) {
	if (targetGear != 99) {
		targetGear = std::clamp(targetGear, -1, GetMaxGear());
	}
	else {
		targetGear = GetMaxGear() + 1;
	}

	if (iniConfig["OPTIONS"]["IMMERSIVE MODE"].as<bool>()) {
		if (IsInAuto[this] == false) {
			return true;
		}
		targetGear = std::clamp(targetGear, 1, GetMaxGear());
	}

	bool bSwitched = Hooked_ShiftGear(this, targetGear);

	if (bSwitched) {
		SetPowerCoef(powerCoef);
	}

	return bSwitched;
}

bool Vehicle::ShiftToNextGear() {
	std::int32_t gear = TruckAction->Gear_1 + 1;

	if (gear == 0 && iniConfig["OPTIONS"]["SKIP NEUTRAL"].as<bool>()) {
		gear = 1;
	}

	return ShiftToGear(std::min(gear, GetMaxGear()));
}

bool Vehicle::ShiftToPrevGear() {
	std::int32_t gear = TruckAction->Gear_1 - 1;

	if (gear == 0 && iniConfig["OPTIONS"]["SKIP NEUTRAL"].as<bool>()) {
		gear = -1;
	}

	if (iniConfig["OPTIONS"]["IMMERSIVE MODE"].as<bool>()) {
		gear = std::max(gear, 1);
	}

	return ShiftToGear(std::max(gear, -1));
}

bool Vehicle::ShiftToHighGear() {
	IsInAuto[this] = true;
	return ShiftToGear(99);
}

bool Vehicle::ShiftToReverseGear() {
	IsInAuto[this] = true;
	return ShiftToGear(-1);
}

bool Vehicle::ShiftToLowGear() {
	IsInAuto[this] = false;
	currentCoef = PowerCoefLowGear;
	return ShiftToGear(1, PowerCoefLowGear);
}

bool Vehicle::ShiftToLowPlusGear() {
	IsInAuto[this] = false;
	currentCoef = PowerCoefLowPlusGear;
	return ShiftToGear(1, PowerCoefLowPlusGear);
}

bool Vehicle::ShiftToLowMinusGear() {
	IsInAuto[this] = false;
	currentCoef = PowerCoefLowMinusGear;
	return ShiftToGear(1, PowerCoefLowMinusGear);
}

bool Hooked_ShiftGear(Vehicle* veh, std::int32_t gear) {
	bool result = ShiftGearO(veh, gear);
	return result;
}

std::int32_t Hooked_GetMaxGear(const Vehicle* veh) {
	return GetMaxGearO(veh);
}

void Hooked_ShiftToAutoGear(Vehicle* veh) {
	veh->TruckAction->IsInAutoMode = false;

	if (iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>()) {
		return;
	}

	IsInAuto[veh] = true;

	if (veh->TruckAction->Gear_1 == (GetMaxGearO(veh) + 1)) {
		veh->ShiftToGear(round(veh->GetMaxGear() * 0.8), 1.0f);
	}
	else if (veh->TruckAction->Gear_1 <= 1) {
		veh->ShiftToGear(1, 1.05f);
	}
	if (!iniConfig["OPTIONS"]["REQUIRE CLUTCH"].as<bool>() && !iniConfig["OPTIONS"]["IMMERSIVE MODE"].as<bool>()) {
		ShiftToAutoGearO(veh);
	}
}

bool Hooked_ShiftToReverse(Vehicle* veh) {
	if (iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>()) {
		return false;
	}

	return ShiftToReverseO(veh);
}

bool Hooked_ShiftToNeutral(Vehicle* veh) {
	if (iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>()) {
		return false;
	}

	return ShiftToNeutralO(veh);
}

bool Hooked_ShiftToHigh(Vehicle* veh) {
	if (iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>()) {
		return false;
	}

	return ShiftToHighO(veh);
}

bool Hooked_DisableAutoAndShift(Vehicle* veh, std::int32_t gear) {
	if (iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>()) {
		return false;
	}

	bool result = DisableAutoAndShiftO(veh, gear);
	IsInAuto[veh] = false;
	return result;
}

void Hooked_SetPowerCoef(Vehicle* veh, float coef) {
	if (iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>()) {
		coef = currentCoef;
	}
	if (IsInAuto[veh]) {
		coef = 1.01;
	}
	SetPowerCoefO(veh, coef);
	ShiftGearO(veh, veh->TruckAction->Gear_2);
}

void Hooked_SetCurrentVehicle(combine_TRUCK_CONTROL* truckCtrl, Vehicle* veh) {
	range = 0;
	if (veh) {
		IsInAuto[veh] = veh->TruckAction->IsInAutoMode;
		if (IsInAuto[veh]) {
			veh->ShiftToGear(1, 1.01);
		}
		veh->TruckAction->IsInAutoMode = false;
	}
	SetCurrentVehicleO(truckCtrl, veh);
}

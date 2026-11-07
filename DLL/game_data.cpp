#include "shared.h"
#include "game_data.h"
#include "memory.h"
#include "config.h"
#include "input.h"

std::atomic<float> currentCoef = 1.f;

std::unordered_map <Vehicle*, std::atomic<bool>> IsInAuto{};

namespace {
	void SelectAuto(Vehicle* veh, bool manual, bool preserveGear) {
		const auto gear = veh->TruckAction->Gear_1;
		// Let the game initialize A's coefficients and choose its entry gear.
		ShiftToAutoGearO(veh);
		veh->TruckAction->IsInAutoMode = !manual;
		IsInAuto[veh] = true;
		if (preserveGear && gear >= -1 && gear <= veh->GetMaxGear()) {
			ShiftGearO(veh, gear);
		}
		else if (iniConfig["OPTIONS"]["PADDLE SHIFT MODE"].as<bool>()) {
			ShiftGearO(veh, 1);
		}
	}
}

void ApplyShiftingOptions() {
	if (auto veh = GetCurrentVehicle(); veh && (IsInAuto[veh] || veh->TruckAction->IsInAutoMode)) {
		const bool paddle = iniConfig["OPTIONS"]["PADDLE SHIFT MODE"].as<bool>();
		const bool manual = paddle ||
			iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>() ||
			iniConfig["OPTIONS"]["IMMERSIVE MODE"].as<bool>() ||
			iniConfig["OPTIONS"]["REQUIRE CLUTCH"].as<bool>();
		SelectAuto(veh, manual, !paddle || veh->TruckAction->Gear_1 > 0);
	}
}

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

	const bool paddle = iniConfig["OPTIONS"]["PADDLE SHIFT MODE"].as<bool>();
	if (paddle) {
		if (!IsInAuto[this]) {
			return false;
		}
		targetGear = std::clamp(targetGear, -1, GetMaxGear());
	}
	else if (iniConfig["OPTIONS"]["IMMERSIVE MODE"].as<bool>()) {
		if (IsInAuto[this] == false) {
			return true;
		}
		targetGear = std::clamp(targetGear, 1, GetMaxGear());
	}

	bool bSwitched = Hooked_ShiftGear(this, targetGear);

	if (bSwitched && !paddle) {
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

	if (!iniConfig["OPTIONS"]["PADDLE SHIFT MODE"].as<bool>() &&
		iniConfig["OPTIONS"]["IMMERSIVE MODE"].as<bool>()) {
		gear = std::max(gear, 1);
	}

	return ShiftToGear(std::max(gear, -1));
}

bool Vehicle::ShiftToHighGear() {
	if (iniConfig["OPTIONS"]["PADDLE SHIFT MODE"].as<bool>()) {
		return Hooked_ShiftToHigh(this);
	}
	IsInAuto[this] = true;
	return ShiftToGear(99);
}

bool Vehicle::ShiftToReverseGear() {
	if (iniConfig["OPTIONS"]["PADDLE SHIFT MODE"].as<bool>()) {
		return Hooked_ShiftToReverse(this);
	}
	IsInAuto[this] = true;
	return ShiftToGear(-1);
}

bool Vehicle::ShiftToLowGear() {
	if (iniConfig["OPTIONS"]["PADDLE SHIFT MODE"].as<bool>()) {
		return false;
	}
	IsInAuto[this] = false;
	currentCoef = PowerCoefLowGear;
	return ShiftToGear(1, PowerCoefLowGear);
}

bool Vehicle::ShiftToLowPlusGear() {
	if (iniConfig["OPTIONS"]["PADDLE SHIFT MODE"].as<bool>()) {
		return false;
	}
	IsInAuto[this] = false;
	currentCoef = PowerCoefLowPlusGear;
	return ShiftToGear(1, PowerCoefLowPlusGear);
}

bool Vehicle::ShiftToLowMinusGear() {
	if (iniConfig["OPTIONS"]["PADDLE SHIFT MODE"].as<bool>()) {
		return false;
	}
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

bool Hooked_IsGearRobot(Vehicle* veh) {
	// Only forward gears use the A display; paddle N/R keep their native labels.
	if (iniConfig["OPTIONS"]["PADDLE SHIFT MODE"].as<bool>() && veh->TruckAction->Gear_2 > 0) {
		const auto selectedAuto = IsInAuto.find(veh);
		if (selectedAuto != IsInAuto.end() && selectedAuto->second) {
			return true;
		}
	}
	return IsGearRobotO(veh);
}

void Hooked_ShiftToAutoGear(Vehicle* veh) {
	if (iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>()) {
		veh->TruckAction->IsInAutoMode = false;
		return;
	}

	const bool paddle = iniConfig["OPTIONS"]["PADDLE SHIFT MODE"].as<bool>();
	const bool manual = paddle || iniConfig["OPTIONS"]["REQUIRE CLUTCH"].as<bool>() ||
		iniConfig["OPTIONS"]["IMMERSIVE MODE"].as<bool>();
	SelectAuto(veh, manual, paddle && IsInAuto[veh]);
}

bool Hooked_ShiftToReverse(Vehicle* veh) {
	if (iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>()) {
		return false;
	}

	IsInAuto[veh] = false;
	return ShiftToReverseO(veh);
}

bool Hooked_ShiftToNeutral(Vehicle* veh) {
	if (iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>()) {
		return false;
	}

	IsInAuto[veh] = false;
	return ShiftToNeutralO(veh);
}

bool Hooked_ShiftToHigh(Vehicle* veh) {
	if (iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>()) {
		return false;
	}

	IsInAuto[veh] = false;
	return ShiftToHighO(veh);
}

bool Hooked_DisableAutoAndShift(Vehicle* veh, std::int32_t gear) {
	if (iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>()) {
		return false;
	}

	// A native range selection can keep the same numbered gear (A1 -> L).
	IsInAuto[veh] = false;
	return DisableAutoAndShiftO(veh, gear);
}

void Hooked_SetPowerCoef(Vehicle* veh, float coef) {
	if (iniConfig["OPTIONS"]["PADDLE SHIFT MODE"].as<bool>()) {
		SetPowerCoefO(veh, coef);
		return;
	}
	if (iniConfig["OPTIONS"]["DISABLE GAME SHIFTING"].as<bool>()) {
		coef = currentCoef;
	}
	SetPowerCoefO(veh, coef);
	ShiftGearO(veh, veh->TruckAction->Gear_2);
}

void Hooked_SetCurrentVehicle(combine_TRUCK_CONTROL* truckCtrl, Vehicle* veh) {
	range = 0;
	SetCurrentVehicleO(truckCtrl, veh);
	if (veh) {
		// A can remain selected even when the engine's automatic mode is off.
		IsInAuto.try_emplace(veh, veh->TruckAction->IsInAutoMode);
		if (veh->TruckAction->IsInAutoMode) {
			IsInAuto[veh] = true;
		}
		if (IsInAuto[veh]) {
			const bool paddle = iniConfig["OPTIONS"]["PADDLE SHIFT MODE"].as<bool>();
			SelectAuto(veh, true, !paddle || veh->TruckAction->Gear_1 > 0);
		}
		veh->TruckAction->IsInAutoMode = false;
	}
}

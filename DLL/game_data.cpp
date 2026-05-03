#include "shared.h"
#include "game_data.h"
#include "memory.h"
#include "config.h"
#include "input.h"
#include <algorithm>
#include <cmath>
#include <mutex>

namespace {
	enum class TransmissionMode {
		Default,
		Low,
		LowPlus,
		LowMinus
	};

	struct ActiveTransmissionState {
		const Vehicle* VehicleRef = nullptr;
		std::int32_t RequestedGear = 1;
		TransmissionMode Mode = TransmissionMode::Default;
		float PowerCoef = PowerCoefDefaultGear;
		bool Hydrated = false;
	};

	std::recursive_mutex transmissionMutex;
	ActiveTransmissionState activeTransmissionState;
	thread_local int32_t modShiftDepth = 0;
	constexpr float PowerCoefTolerance = 0.05f;
	bool EnsureActiveTransmissionStateForVehicleUnlocked(Vehicle* veh);

	class ScopedModShift {
	public:
		ScopedModShift() {
			++modShiftDepth;
		}

		~ScopedModShift() {
			--modShiftDepth;
		}
	};

	bool IsModShiftActive() {
		return modShiftDepth > 0;
	}

	bool ApproximatelyEqual(float lhs, float rhs) {
		return std::fabs(lhs - rhs) <= PowerCoefTolerance;
	}

	bool UsesDefaultPower(const ActiveTransmissionState& state) {
		return state.Mode == TransmissionMode::Default;
	}

	const char* TransmissionModeName(TransmissionMode mode) {
		switch (mode) {
		case TransmissionMode::Default:
			return "DEFAULT";
		case TransmissionMode::Low:
			return "LOW";
		case TransmissionMode::LowPlus:
			return "LOW_PLUS";
		case TransmissionMode::LowMinus:
			return "LOW_MINUS";
		}
		return "UNKNOWN";
	}

	float EffectivePowerCoef(const ActiveTransmissionState& state) {
		return UsesDefaultPower(state) ? PowerCoefDefaultGear : state.PowerCoef;
	}

	void ResetActiveTransmissionState(const Vehicle* veh) {
		activeTransmissionState.VehicleRef = veh;
		activeTransmissionState.RequestedGear = 1;
		activeTransmissionState.Mode = TransmissionMode::Default;
		activeTransmissionState.PowerCoef = PowerCoefDefaultGear;
		activeTransmissionState.Hydrated = veh != nullptr;
	}

	std::int32_t HighGearValue(const Vehicle* veh) {
		return veh->GetMaxGear() + 1;
	}

	std::int32_t NormalizeRequestedGear(const Vehicle* veh, std::int32_t requestedGear) {
		if (requestedGear == 99) {
			return HighGearValue(veh);
		}

		return std::clamp(requestedGear, -1, veh->GetMaxGear());
	}

	std::int32_t RequestedGearFromGameGear(const Vehicle* veh, std::int32_t gear) {
		return gear > veh->GetMaxGear() ? 99 : std::clamp(gear, -1, veh->GetMaxGear());
	}

	std::int32_t CurrentRequestedTargetGearUnlocked(Vehicle* veh) {
		if (!EnsureActiveTransmissionStateForVehicleUnlocked(veh)) {
			return 0;
		}

		return NormalizeRequestedGear(veh, activeTransmissionState.RequestedGear);
	}

	void MirrorManualGearState(Vehicle* veh, std::int32_t targetGear) {
		if (veh == nullptr || veh->TruckAction == nullptr) {
			return;
		}

		veh->TruckAction->IsInAutoMode = false;
		veh->TruckAction->Gear_2 = targetGear;
		veh->TruckAction->NextGear = targetGear;
	}

	bool ShouldAllowNativeAutoShifting();
	bool ShouldForceManualDriveState();

	bool IsInNativeAutoState(const Vehicle* veh) {
		return ShouldAllowNativeAutoShifting()
			&& veh != nullptr
			&& veh->TruckAction != nullptr
			&& veh->TruckAction->IsInAutoMode;
	}

	bool ShouldPreserveManualControllerState(const Vehicle* veh) {
		return ShouldForceManualDriveState()
			|| (veh != nullptr
				&& veh->TruckAction != nullptr
				&& !veh->TruckAction->IsInAutoMode);
	}

	void SyncActiveTransmissionStateFromVehicleUnlocked(Vehicle* veh) {
		const ActiveTransmissionState previousState = activeTransmissionState;
		ResetActiveTransmissionState(veh);
		if (veh == nullptr || veh->TruckAction == nullptr) {
			return;
		}

		const std::int32_t maxGear = veh->GetMaxGear();
		const std::int32_t actualGear = veh->TruckAction->Gear_1;
		const float powerCoef = veh->TruckAction->PowerCoef;

		if (ApproximatelyEqual(powerCoef, PowerCoefLowGear)) {
			activeTransmissionState.Mode = TransmissionMode::Low;
			activeTransmissionState.PowerCoef = PowerCoefLowGear;
			activeTransmissionState.RequestedGear = 1;
			return;
		}
		if (previousState.VehicleRef == veh
			&& previousState.Hydrated
			&& previousState.Mode == TransmissionMode::LowPlus
			&& actualGear == 1
			&& ApproximatelyEqual(powerCoef, PowerCoefLowPlusGear)
			&& ShouldPreserveManualControllerState(veh)) {
			activeTransmissionState.Mode = TransmissionMode::LowPlus;
			activeTransmissionState.PowerCoef = PowerCoefLowPlusGear;
			activeTransmissionState.RequestedGear = 1;
			return;
		}
		if (ApproximatelyEqual(powerCoef, PowerCoefLowMinusGear)) {
			activeTransmissionState.Mode = TransmissionMode::LowMinus;
			activeTransmissionState.PowerCoef = PowerCoefLowMinusGear;
			activeTransmissionState.RequestedGear = 1;
			return;
		}

		activeTransmissionState.Mode = TransmissionMode::Default;
		activeTransmissionState.PowerCoef = PowerCoefDefaultGear;
		activeTransmissionState.RequestedGear = RequestedGearFromGameGear(veh, actualGear);
		if (ShouldPreserveManualControllerState(veh)) {
			const std::int32_t pendingGear = RequestedGearFromGameGear(veh, veh->TruckAction->NextGear);
			if (pendingGear != activeTransmissionState.RequestedGear) {
				activeTransmissionState.RequestedGear = pendingGear;
			}
		}
	}

	bool EnsureActiveTransmissionStateForVehicleUnlocked(Vehicle* veh) {
		if (veh == nullptr) {
			return false;
		}

		if (activeTransmissionState.VehicleRef != veh || !activeTransmissionState.Hydrated) {
			SyncActiveTransmissionStateFromVehicleUnlocked(veh);
		}

		return true;
	}

	std::int32_t CurrentLogicalGearUnlocked(Vehicle* veh) {
		if (!EnsureActiveTransmissionStateForVehicleUnlocked(veh)) {
			return 0;
		}

		if (activeTransmissionState.RequestedGear == 99) {
			return HighGearValue(veh);
		}

		return NormalizeRequestedGear(veh, activeTransmissionState.RequestedGear);
	}

	void SetDefaultRequestedGearUnlocked(Vehicle* veh, std::int32_t requestedGear) {
		EnsureActiveTransmissionStateForVehicleUnlocked(veh);
		activeTransmissionState.Mode = TransmissionMode::Default;
		activeTransmissionState.PowerCoef = PowerCoefDefaultGear;
		activeTransmissionState.RequestedGear = requestedGear;
	}

	void SetLowRequestedGearUnlocked(Vehicle* veh, TransmissionMode mode, float powerCoef) {
		EnsureActiveTransmissionStateForVehicleUnlocked(veh);
		activeTransmissionState.Mode = mode;
		activeTransmissionState.PowerCoef = powerCoef;
		activeTransmissionState.RequestedGear = 1;
	}

	bool ApplyRequestedGearNativeUnlocked(Vehicle* veh, std::int32_t targetGear) {
		if (veh == nullptr) {
			return false;
		}

		const std::int32_t maxGear = veh->GetMaxGear();
		if (targetGear > maxGear) {
			return ShiftToHighO(veh);
		}
		if (targetGear < 0) {
			return ShiftToReverseO(veh);
		}
		if (targetGear == 0) {
			return ShiftToNeutralO(veh);
		}
		return DisableAutoAndShiftO(veh, targetGear);
	}

	bool ApplyActiveTransmissionStateUnlocked(Vehicle* veh) {
		if (!EnsureActiveTransmissionStateForVehicleUnlocked(veh)) {
			return false;
		}

		const std::int32_t targetGear = NormalizeRequestedGear(veh, activeTransmissionState.RequestedGear);
		const float powerCoef = EffectivePowerCoef(activeTransmissionState);
		const std::int32_t actualGearBefore = veh->TruckAction != nullptr ? veh->TruckAction->Gear_1 : 0;

		DebugLogMessage(
			"ApplyTransmission",
			"veh", static_cast<const void*>(veh),
			"requested", activeTransmissionState.RequestedGear,
			"target", targetGear,
			"mode", TransmissionModeName(activeTransmissionState.Mode),
			"coef", powerCoef,
			"actual_before", actualGearBefore
		);

		ScopedModShift modShiftScope;
		const bool shifted = ApplyRequestedGearNativeUnlocked(veh, targetGear);
		SetPowerCoefO(veh, powerCoef);
		MirrorManualGearState(veh, targetGear);

		SyncActiveTransmissionStateFromVehicleUnlocked(veh);
		DebugLogMessage(
			"ApplyTransmissionResult",
			"veh", static_cast<const void*>(veh),
			"shifted", shifted,
			"actual_after", veh->TruckAction != nullptr ? veh->TruckAction->Gear_1 : 0,
			"next_after", veh->TruckAction != nullptr ? veh->TruckAction->NextGear : 0,
			"gear2_after", veh->TruckAction != nullptr ? veh->TruckAction->Gear_2 : 0,
			"mode_after", TransmissionModeName(activeTransmissionState.Mode),
			"requested_after", activeTransmissionState.RequestedGear
		);
		return shifted || (veh->TruckAction != nullptr && veh->TruckAction->Gear_1 == targetGear);
	}

	void EnforceActiveTransmissionStateUnlocked(Vehicle* veh) {
		if (!EnsureActiveTransmissionStateForVehicleUnlocked(veh)) {
			return;
		}

		SetPowerCoefO(veh, EffectivePowerCoef(activeTransmissionState));
		MirrorManualGearState(veh, CurrentRequestedTargetGearUnlocked(veh));
	}

	bool ShouldBlockGameShifting() {
		return GetRuntimeOption(RuntimeOption::DisableGameShifting) && !IsModShiftActive();
	}

	bool ShouldAllowNativeAutoShifting() {
		return !GetRuntimeOption(RuntimeOption::DisableGameShifting)
			&& !GetRuntimeOption(RuntimeOption::ImmersiveMode)
			&& !GetRuntimeOption(RuntimeOption::RequireClutch);
	}

	bool ShouldForceManualDriveState() {
		return !ShouldAllowNativeAutoShifting();
	}
}

bool GetIsInAuto(const Vehicle* veh) {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	if (veh == nullptr) {
		return false;
	}

	EnsureActiveTransmissionStateForVehicleUnlocked(const_cast<Vehicle*>(veh));
	return UsesDefaultPower(activeTransmissionState);
}

void SetIsInAuto(const Vehicle* veh, bool value) {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	Vehicle* vehicle = const_cast<Vehicle*>(veh);
	if (!EnsureActiveTransmissionStateForVehicleUnlocked(vehicle)) {
		return;
	}

	if (value) {
		activeTransmissionState.Mode = TransmissionMode::Default;
		activeTransmissionState.PowerCoef = PowerCoefDefaultGear;
		return;
	}

	activeTransmissionState.Mode = TransmissionMode::Low;
	activeTransmissionState.PowerCoef = PowerCoefLowGear;
	activeTransmissionState.RequestedGear = 1;
}

void SetDefaultGearState(const Vehicle* veh) {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	Vehicle* vehicle = const_cast<Vehicle*>(veh);
	if (!EnsureActiveTransmissionStateForVehicleUnlocked(vehicle)) {
		return;
	}

	activeTransmissionState.Mode = TransmissionMode::Default;
	activeTransmissionState.PowerCoef = PowerCoefDefaultGear;
}

void SyncVehicleStateFromTruckAction(Vehicle* veh) {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	SyncActiveTransmissionStateFromVehicleUnlocked(veh);
	DebugLogMessage(
		"SyncVehicleState",
		"veh", static_cast<const void*>(veh),
		"gear", veh != nullptr && veh->TruckAction != nullptr ? veh->TruckAction->Gear_1 : 0,
		"power", veh != nullptr && veh->TruckAction != nullptr ? veh->TruckAction->PowerCoef : 0.0f,
		"mode", TransmissionModeName(activeTransmissionState.Mode),
		"requested", activeTransmissionState.RequestedGear
	);
}

void Vehicle::SetPowerCoef(float coef) { Hooked_SetPowerCoef(this, coef); }

std::int32_t Vehicle::GetMaxGear() const {
	return GetMaxGearO(this);
}

bool Vehicle::ShiftToGear(std::int32_t targetGear, float powerCoef) {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	DebugLogMessage(
		"ShiftToGearRequest",
		"veh", static_cast<const void*>(this),
		"target", targetGear,
		"power", powerCoef,
		"immersive", GetRuntimeOption(RuntimeOption::ImmersiveMode)
	);

	if (IsInNativeAutoState(this)) {
		DebugLogMessage(
			"IgnoreModShiftInNativeAuto",
			"veh", static_cast<const void*>(this),
			"action", "ShiftToGear",
			"target", targetGear
		);
		return false;
	}

	const bool wantsLowGear = targetGear == 1 && ApproximatelyEqual(powerCoef, PowerCoefLowGear);
	const bool wantsLowPlusGear = targetGear == 1 && ApproximatelyEqual(powerCoef, PowerCoefLowPlusGear);
	const bool wantsLowMinusGear = targetGear == 1 && ApproximatelyEqual(powerCoef, PowerCoefLowMinusGear);

	if (GetRuntimeOption(RuntimeOption::ImmersiveMode) && (wantsLowGear || wantsLowPlusGear || wantsLowMinusGear)) {
		return true;
	}

	if (wantsLowGear) {
		SetLowRequestedGearUnlocked(this, TransmissionMode::Low, PowerCoefLowGear);
	}
	else if (wantsLowPlusGear) {
		SetLowRequestedGearUnlocked(this, TransmissionMode::LowPlus, PowerCoefLowPlusGear);
	}
	else if (wantsLowMinusGear) {
		SetLowRequestedGearUnlocked(this, TransmissionMode::LowMinus, PowerCoefLowMinusGear);
	}
	else {
		if (GetRuntimeOption(RuntimeOption::ImmersiveMode)) {
			targetGear = std::clamp(targetGear, 1, GetMaxGear());
		}

		SetDefaultRequestedGearUnlocked(this, targetGear);
	}

	return ApplyActiveTransmissionStateUnlocked(this);
}

bool Vehicle::ShiftToNextGear() {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	if (!EnsureActiveTransmissionStateForVehicleUnlocked(this)) {
		return false;
	}

	if (IsInNativeAutoState(this)) {
		DebugLogMessage(
			"IgnoreModShiftInNativeAuto",
			"veh", static_cast<const void*>(this),
			"action", "ShiftToNextGear",
			"current", this->TruckAction != nullptr ? this->TruckAction->Gear_1 : 0
		);
		return false;
	}

	const std::int32_t maxGear = GetMaxGear();
	std::int32_t currentGear = CurrentLogicalGearUnlocked(this);
	if (currentGear > maxGear) {
		currentGear = maxGear;
	}

	std::int32_t nextGear = currentGear + 1;
	if (nextGear == 0 && GetRuntimeOption(RuntimeOption::SkipNeutral)) {
		nextGear = 1;
	}

	DebugLogMessage(
		"ShiftUpRequest",
		"veh", static_cast<const void*>(this),
		"current", currentGear,
		"next", nextGear
	);
	SetDefaultRequestedGearUnlocked(this, std::min(nextGear, maxGear));
	return ApplyActiveTransmissionStateUnlocked(this);
}

bool Vehicle::ShiftToPrevGear() {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	if (!EnsureActiveTransmissionStateForVehicleUnlocked(this)) {
		return false;
	}

	if (IsInNativeAutoState(this)) {
		DebugLogMessage(
			"IgnoreModShiftInNativeAuto",
			"veh", static_cast<const void*>(this),
			"action", "ShiftToPrevGear",
			"current", this->TruckAction != nullptr ? this->TruckAction->Gear_1 : 0
		);
		return false;
	}

	const std::int32_t maxGear = GetMaxGear();
	const std::int32_t currentGear = CurrentLogicalGearUnlocked(this);
	std::int32_t prevGear = currentGear > maxGear ? maxGear : currentGear - 1;

	if (prevGear == 0 && GetRuntimeOption(RuntimeOption::SkipNeutral)) {
		prevGear = -1;
	}

	if (GetRuntimeOption(RuntimeOption::ImmersiveMode)) {
		prevGear = std::max(prevGear, 1);
	}

	DebugLogMessage(
		"ShiftDownRequest",
		"veh", static_cast<const void*>(this),
		"current", currentGear,
		"prev", prevGear
	);
	SetDefaultRequestedGearUnlocked(this, std::max(prevGear, -1));
	return ApplyActiveTransmissionStateUnlocked(this);
}

bool Vehicle::ShiftToHighGear() {
	return ShiftToGear(99, PowerCoefDefaultGear);
}

bool Vehicle::ShiftToReverseGear() {
	return ShiftToGear(-1, PowerCoefDefaultGear);
}

bool Vehicle::ShiftToLowGear() {
	return ShiftToGear(1, PowerCoefLowGear);
}

bool Vehicle::ShiftToLowPlusGear() {
	return ShiftToGear(1, PowerCoefLowPlusGear);
}

bool Vehicle::ShiftToLowMinusGear() {
	return ShiftToGear(1, PowerCoefLowMinusGear);
}

bool Hooked_ShiftGear(Vehicle* veh, std::int32_t gear) {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	if (ShouldBlockGameShifting()) {
		EnsureActiveTransmissionStateForVehicleUnlocked(veh);
		DebugLogMessage("BlockGameShiftGear", "veh", static_cast<const void*>(veh), "gear", gear);
		MirrorManualGearState(veh, CurrentRequestedTargetGearUnlocked(veh));
		return false;
	}

	DebugLogMessage("PassGameShiftGear", "veh", static_cast<const void*>(veh), "gear", gear);
	const bool result = ShiftGearO(veh, gear);
	SyncActiveTransmissionStateFromVehicleUnlocked(veh);
	return result;
}

std::int32_t Hooked_GetMaxGear(const Vehicle* veh) {
	return GetMaxGearO(veh);
}

void Hooked_ShiftToAutoGear(Vehicle* veh) {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	if (ShouldBlockGameShifting()) {
		DebugLogMessage("BlockShiftToAuto", "veh", static_cast<const void*>(veh));
		EnforceActiveTransmissionStateUnlocked(veh);
		return;
	}

	if (ShouldAllowNativeAutoShifting()) {
		DebugLogMessage("AllowNativeAuto", "veh", static_cast<const void*>(veh));
		ShiftToAutoGearO(veh);
		SyncActiveTransmissionStateFromVehicleUnlocked(veh);
		return;
	}

	DebugLogMessage("ForceManualAutoPath", "veh", static_cast<const void*>(veh));
	SyncActiveTransmissionStateFromVehicleUnlocked(veh);
	if (veh != nullptr && veh->TruckAction != nullptr) {
		veh->TruckAction->IsInAutoMode = false;
	}
}

bool Hooked_ShiftToReverse(Vehicle* veh) {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	if (ShouldBlockGameShifting()) {
		EnforceActiveTransmissionStateUnlocked(veh);
		return false;
	}

	const bool result = ShiftToReverseO(veh);
	SyncActiveTransmissionStateFromVehicleUnlocked(veh);
	if (ShouldForceManualDriveState()) {
		veh->TruckAction->IsInAutoMode = false;
	}
	return result;
}

bool Hooked_ShiftToNeutral(Vehicle* veh) {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	if (ShouldBlockGameShifting()) {
		EnforceActiveTransmissionStateUnlocked(veh);
		return false;
	}

	const bool result = ShiftToNeutralO(veh);
	SyncActiveTransmissionStateFromVehicleUnlocked(veh);
	if (ShouldForceManualDriveState()) {
		veh->TruckAction->IsInAutoMode = false;
	}
	return result;
}

bool Hooked_ShiftToHigh(Vehicle* veh) {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	if (ShouldBlockGameShifting()) {
		EnforceActiveTransmissionStateUnlocked(veh);
		return false;
	}

	const bool result = ShiftToHighO(veh);
	SyncActiveTransmissionStateFromVehicleUnlocked(veh);
	if (ShouldForceManualDriveState()) {
		veh->TruckAction->IsInAutoMode = false;
	}
	return result;
}

bool Hooked_DisableAutoAndShift(Vehicle* veh, std::int32_t gear) {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	if (ShouldBlockGameShifting()) {
		EnforceActiveTransmissionStateUnlocked(veh);
		return false;
	}

	const bool result = DisableAutoAndShiftO(veh, gear);
	SyncActiveTransmissionStateFromVehicleUnlocked(veh);
	if (ShouldForceManualDriveState()) {
		veh->TruckAction->IsInAutoMode = false;
	}
	return result;
}

void Hooked_SetPowerCoef(Vehicle* veh, float coef) {
	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	if (ShouldBlockGameShifting()) {
		DebugLogMessage("BlockSetPowerCoef", "veh", static_cast<const void*>(veh), "requested_coef", coef);
		EnforceActiveTransmissionStateUnlocked(veh);
		return;
	}

	DebugLogMessage("PassSetPowerCoef", "veh", static_cast<const void*>(veh), "coef", coef);
	SetPowerCoefO(veh, coef);
	SyncActiveTransmissionStateFromVehicleUnlocked(veh);
}

void Hooked_SetCurrentVehicle(combine_TRUCK_CONTROL* truckCtrl, Vehicle* veh) {
	range = 0;
	SetCurrentVehicleO(truckCtrl, veh);

	std::lock_guard<std::recursive_mutex> lock(transmissionMutex);
	SyncActiveTransmissionStateFromVehicleUnlocked(veh);
	DebugLogMessage(
		"SetCurrentVehicle",
		"veh", static_cast<const void*>(veh),
		"block_game_shifting", ShouldBlockGameShifting(),
		"allow_native_auto", ShouldAllowNativeAutoShifting(),
		"gear", veh != nullptr && veh->TruckAction != nullptr ? veh->TruckAction->Gear_1 : 0,
		"power", veh != nullptr && veh->TruckAction != nullptr ? veh->TruckAction->PowerCoef : 0.0f
	);
	if (ShouldBlockGameShifting()) {
		EnforceActiveTransmissionStateUnlocked(veh);
		return;
	}

	if (ShouldForceManualDriveState() && veh != nullptr && veh->TruckAction != nullptr) {
		veh->TruckAction->IsInAutoMode = false;
	}
}

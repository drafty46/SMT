#pragma once
#include "shared.h"
#include "game_data.h"

extern void DetachDLL();

extern combine_TRUCK_CONTROL** TruckControlPtr;

using Fnc_ShiftGear = bool(Vehicle* veh, std::int32_t gear);
extern Fnc_ShiftGear* ShiftGearO;
extern bool Hooked_ShiftGear(Vehicle* veh, std::int32_t gear);

using Fnc_ShiftToAutoGear = void(Vehicle* veh);
extern Fnc_ShiftToAutoGear* ShiftToAutoGearO;
extern void Hooked_ShiftToAutoGear(Vehicle* veh);

using Fnc_ShiftToHigh = bool(Vehicle* veh);
extern Fnc_ShiftToHigh* ShiftToHighO;
extern bool Hooked_ShiftToHigh(Vehicle* veh);

using Fnc_ShiftToReverse = bool(Vehicle* veh);
extern Fnc_ShiftToReverse* ShiftToReverseO;
extern bool Hooked_ShiftToReverse(Vehicle* veh);

using Fnc_ShiftToNeutral = bool(Vehicle* veh);
extern Fnc_ShiftToNeutral* ShiftToNeutralO;
extern bool Hooked_ShiftToNeutral(Vehicle* veh);

using Fnc_GetMaxGear = std::int32_t(const Vehicle* veh);
extern Fnc_GetMaxGear* GetMaxGearO;
extern std::int32_t Hooked_GetMaxGear(const Vehicle* veh);

using Fnc_DisableAutoAndShift = bool(Vehicle* veh, std::int32_t gear);
extern Fnc_DisableAutoAndShift* DisableAutoAndShiftO;
extern bool Hooked_DisableAutoAndShift(Vehicle* veh, std::int32_t gear);

using Fnc_SetPowerCoef = void(Vehicle* veh, float coef);
extern Fnc_SetPowerCoef* SetPowerCoefO;
extern void Hooked_SetPowerCoef(Vehicle* veh, float coef);

using Fnc_SetCurrentVehicle = void(combine_TRUCK_CONTROL* truckCtrl, Vehicle* veh);
extern Fnc_SetCurrentVehicle* SetCurrentVehicleO;
extern void Hooked_SetCurrentVehicle(combine_TRUCK_CONTROL* truckCtrl, Vehicle* veh);

extern void InitMemory();
extern void ShutdownMemory();
extern std::atomic<bool> foundOffsets;

extern Vehicle* GetCurrentVehicle();
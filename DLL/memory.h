#pragma once
#include "shared.h"
#include "game_data.h"

extern void DetachDLL();

class Vehicle;
class combine_TRUCK_CONTROL;

extern combine_TRUCK_CONTROL* TruckControlPtr;

using Fnc_ShiftGear = bool(Vehicle*, std::int32_t);
extern Fnc_ShiftGear* ShiftGearO;
extern bool Hooked_ShiftGear(Vehicle* veh, std::int32_t gear);

using Fnc_ShiftToAutoGear = void(Vehicle*);
extern Fnc_ShiftToAutoGear* ShiftToAutoGearO;
extern void Hooked_ShiftToAutoGear(Vehicle* veh);

using Fnc_ShiftToHigh = bool(Vehicle*);
extern Fnc_ShiftToHigh* ShiftToHighO;
extern bool Hooked_ShiftToHigh(Vehicle* veh);

using Fnc_ShiftToReverse = bool(Vehicle*);
extern Fnc_ShiftToReverse* ShiftToReverseO;
extern bool Hooked_ShiftToReverse(Vehicle* veh);

using Fnc_ShiftToNeutral = bool(Vehicle*);
extern Fnc_ShiftToNeutral* ShiftToNeutralO;
extern bool Hooked_ShiftToNeutral(Vehicle* veh);

using Fnc_GetMaxGear = std::int32_t(const Vehicle*);
extern Fnc_GetMaxGear* GetMaxGearO;
extern std::int32_t Hooked_GetMaxGear(const Vehicle* veh);

using Fnc_DisableAutoAndShift = bool(Vehicle*, std::int32_t);
extern Fnc_DisableAutoAndShift* DisableAutoAndShiftO;
extern bool Hooked_DisableAutoAndShift(Vehicle* veh, std::int32_t gear);

using Fnc_SetPowerCoef = void(Vehicle*, float);
extern Fnc_SetPowerCoef* SetPowerCoefO;
extern void Hooked_SetPowerCoef(Vehicle* veh, float coef);

using Fnc_SetCurrentVehicle = void(combine_TRUCK_CONTROL*, Vehicle*);
extern Fnc_SetCurrentVehicle* SetCurrentVehicleO;
extern void Hooked_SetCurrentVehicle(combine_TRUCK_CONTROL* truckCtrl, Vehicle* veh);

extern void InitMemory();
extern void ShutdownMemory();
extern std::atomic<bool> foundOffsets;


#ifndef CANSM_H
#define CANSM_H

#include "Std_Types.h"
#include "CanTypes.h"

void CanSM_Init(void);
void CanSM_MainFunction(void);

Std_ReturnType_t CanSM_RequestState(CanSM_NetworkState_t targetState);
CanSM_NetworkState_t CanSM_GetCurrentState(void);

// Event notifications from lower layers (e.g., CanIf / CanTrcv)
void CanSM_ReportBusWakeup(void);
void CanSM_ReportBusOff(void);

#endif // CANSM_H
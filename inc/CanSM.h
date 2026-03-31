#ifndef CANSM_H
#define CANSM_H

#include "Std_Types.h"
#include "CanTypes.h"

typedef enum {
    CANSM_TIMER_BUS_OFF,
    CANSM_TIMER_NO
} CanSM_TimerId_t;

#define CANSM_TIMER_BUS_OFF_FAST_RECOVERY_MAX (CAN_TIMER_BUS_OFF_FAST_RECOVERY / CAN_MAIN_FUNCTION_PERIOD)
#define CANSM_TIMER_BUS_OFF_SLOW_RECOVERY_MAX (CAN_TIMER_BUS_OFF_SLOW_RECOVERY / CAN_MAIN_FUNCTION_PERIOD)

void CanSM_Init(void);
void CanSM_MainFunction(void);
Std_ReturnType_t CanSM_RequestState(CanSM_NetworkState_t targetState);
CanSM_NetworkState_t CanSM_GetCurrentState(void);
void CanSM_ReportBusWakeup(void);
void CanSM_ReportBusOff(void);

void CanSM_StartTimer(uint8_t network, CanSM_TimerId_t timerId, uint32_t value);
bool CanSM_GetTimer(uint8_t network, CanSM_TimerId_t timerId);

#endif // CANSM_H
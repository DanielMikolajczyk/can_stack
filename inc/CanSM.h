#ifndef CANSM_H
#define CANSM_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CANSM_STATE_UNINIT,
    CANSM_STATE_OFFLINE,
    CANSM_STATE_ONLINE,
    CANSM_STATE_SLEEP,
    CANSM_STATE_BUS_OFF
} CanSM_NetworkState_t;

void CanSM_Init(void);
void CanSM_MainFunction(void);

bool CanSM_RequestState(CanSM_NetworkState_t targetState);
CanSM_NetworkState_t CanSM_GetCurrentState(void);

// Event notifications from lower layers (e.g., CanIf / CanTrcv)
void CanSM_ReportBusWakeup(void);
void CanSM_ReportBusOff(void);

#endif // CANSM_H
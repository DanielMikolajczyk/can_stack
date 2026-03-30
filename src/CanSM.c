#include "CanSM.h"
#include "CanIf.h"

STATIC CanSM_NetworkState_t CanSM_CurrentState = CANSM_STATE_UNINIT;
STATIC CanSM_NetworkState_t CanSM_RequestedState = CANSM_STATE_UNINIT;

// Flags for asynchronous hardware events.
// In a real environment, these are usually set via ISRs, so consider atomic blocks if needed.
STATIC bool CanSM_BusWakeupPending = false;
STATIC bool CanSM_BusOffPending = false;

void CanSM_Init(void) {
    CanSM_CurrentState = CANSM_STATE_OFFLINE;
    CanSM_RequestedState = CANSM_STATE_OFFLINE;
    CanSM_BusWakeupPending = false;
    CanSM_BusOffPending = false;
}

Std_ReturnType_t CanSM_RequestState(CanSM_NetworkState_t targetState) {
    Std_ReturnType_t ret_val = E_NOT_OK;
    if (CANSM_STATE_UNINIT != CanSM_CurrentState) {
        /* Cannot request state transitions before initialization */
        ret_val = E_OK;
    }
    CanSM_RequestedState = targetState;
    return ret_val;
}

CanSM_NetworkState_t CanSM_GetCurrentState(void) {
    return CanSM_CurrentState;
}

void CanSM_ReportBusWakeup(void) {
    CanSM_BusWakeupPending = true;
}

void CanSM_ReportBusOff(void) {
    CanSM_BusOffPending = true;
}

void CanSM_MainFunction(void) {
    if (CanSM_CurrentState == CANSM_STATE_UNINIT) {
        return;
    }

    // 1. Process Hardware Events (Highest Priority)
    if (CanSM_BusOffPending) {
        CanSM_BusOffPending = false;
        CanSM_CurrentState = CANSM_STATE_BUS_OFF;
        CanSM_RequestedState = CANSM_STATE_OFFLINE; // Reset request to force a controlled recovery later
        CanIf_SetControllerMode(0, CANIF_CS_STOPPED); // Assuming Controller 0
    }

    if (CanSM_BusWakeupPending) {
        CanSM_BusWakeupPending = false;
        if (CanSM_CurrentState == CANSM_STATE_SLEEP) {
            CanSM_CurrentState = CANSM_STATE_ONLINE;
            CanSM_RequestedState = CANSM_STATE_ONLINE;
            // TODO: Command CanTrcv to NORMAL
            CanIf_SetControllerMode(0, CANIF_CS_STARTED);
        }
    }

    // 2. Process Requested State Transitions
    if (CanSM_RequestedState != CanSM_CurrentState) {
        switch (CanSM_RequestedState) {
            case CANSM_STATE_ONLINE:
                if ((CANSM_STATE_OFFLINE == CanSM_CurrentState) || (CANSM_STATE_SLEEP == CanSM_CurrentState)) {
                    CanIf_SetControllerMode(0, CANIF_CS_STARTED);
                    CanSM_CurrentState = CANSM_STATE_ONLINE;
                }
                break;
            case CANSM_STATE_OFFLINE:
                CanIf_SetControllerMode(0, CANIF_CS_STOPPED);
                CanSM_CurrentState = CANSM_STATE_OFFLINE;
                break;
            case CANSM_STATE_SLEEP:
                if ((CANSM_STATE_OFFLINE == CanSM_CurrentState) || (CANSM_STATE_ONLINE == CanSM_CurrentState)) {
                    CanIf_SetControllerMode(0, CANIF_CS_SLEEP);
                    CanSM_CurrentState = CANSM_STATE_SLEEP;
                }
                break;
            default: break;
        }
    }
}
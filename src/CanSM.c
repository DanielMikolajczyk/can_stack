#include "CanSM.h"
#include <stddef.h>

static CanSM_NetworkState_t currentState = CANSM_STATE_UNINIT;
static CanSM_NetworkState_t requestedState = CANSM_STATE_UNINIT;

// Flags for asynchronous hardware events.
// In a real environment, these are usually set via ISRs, so consider atomic blocks if needed.
static bool busWakeupPending = false;
static bool busOffPending = false;

void CanSM_Init(void) {
    currentState = CANSM_STATE_OFFLINE;
    requestedState = CANSM_STATE_OFFLINE;
    busWakeupPending = false;
    busOffPending = false;
}

bool CanSM_RequestState(CanSM_NetworkState_t targetState) {
    if (currentState == CANSM_STATE_UNINIT) {
        return false; // Cannot request state transitions before initialization
    }
    requestedState = targetState;
    return true;
}

CanSM_NetworkState_t CanSM_GetCurrentState(void) {
    return currentState;
}

void CanSM_ReportBusWakeup(void) {
    busWakeupPending = true;
}

void CanSM_ReportBusOff(void) {
    busOffPending = true;
}

void CanSM_MainFunction(void) {
    if (currentState == CANSM_STATE_UNINIT) {
        return;
    }

    // 1. Process Hardware Events (Highest Priority)
    if (busOffPending) {
        busOffPending = false;
        currentState = CANSM_STATE_BUS_OFF;
        requestedState = CANSM_STATE_OFFLINE; // Reset request to force a controlled recovery later
        // TODO: Command CanIf to stop TX/RX immediately
    }

    if (busWakeupPending) {
        busWakeupPending = false;
        if (currentState == CANSM_STATE_SLEEP) {
            currentState = CANSM_STATE_ONLINE;
            requestedState = CANSM_STATE_ONLINE;
            // TODO: Command CanTrcv to NORMAL, CanIf to ONLINE
        }
    }

    // 2. Process Requested State Transitions
    if (requestedState != currentState) {
        switch (requestedState) {
            case CANSM_STATE_ONLINE:
                if (currentState == CANSM_STATE_OFFLINE || currentState == CANSM_STATE_SLEEP) {
                    // TODO: Call CanTrcv and CanIf to enable hardware
                    currentState = CANSM_STATE_ONLINE;
                }
                break;
            case CANSM_STATE_OFFLINE:
                // TODO: Call CanIf to disable TX/RX
                currentState = CANSM_STATE_OFFLINE;
                break;
            case CANSM_STATE_SLEEP:
                if (currentState == CANSM_STATE_OFFLINE || currentState == CANSM_STATE_ONLINE) {
                    // TODO: Call CanTrcv to go to standby/sleep mode
                    currentState = CANSM_STATE_SLEEP;
                }
                break;
            default: break;
        }
    }
}
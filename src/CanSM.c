#include "CanSM.h"
#include "CanIf.h"
#include "CanCfg.h"

#define CANSM_MAX_TIMERS    (1u)

STATIC CanSM_NetworkState_t CanSM_CurrentState = CANSM_STATE_UNINIT;
STATIC CanSM_NetworkState_t CanSM_RequestedState = CANSM_STATE_UNINIT;

// Flags for asynchronous hardware events.
// In a real environment, these are usually set via ISRs, so consider atomic blocks if needed.
STATIC bool CanSM_BusWakeupPending = false;
STATIC bool CanSM_BusOffPending = false;

STATIC uint32_t CanSM_Timer[CAN_NETWORKS_NO][CANSM_MAX_TIMERS];
STATIC bool CanSM_TimerActive[CAN_NETWORKS_NO][CANSM_MAX_TIMERS];

STATIC void CanSM_BusOffRecovery(void);

/**
 * @brief Handles the Bus-Off recovery sequence.
 * @details This static function is called periodically by CanSM_MainFunction.
 *          It manages the Bus-Off recovery timer and attempts to bring the CAN controller
 *          back to the STARTED state after the timer expires. It also handles retries if the initial attempt fails.
 */
STATIC void CanSM_BusOffRecovery(void) {
    /* Decrement active timers on every periodic cycle */
    for (uint8_t net = 0; net < CAN_NETWORKS_NO; net++) {
        for (uint8_t t = 0; t < CANSM_MAX_TIMERS; t++) {
            if (CanSM_TimerActive[net][t] && (CanSM_Timer[net][t] > 0u)) {
                CanSM_Timer[net][t]--;
            }
        }
    }

    if (CanSM_GetTimer(0, CANSM_TIMER_BUS_OFF)) {
        if (E_NOT_OK == CanIf_SetControllerMode(0, CANIF_CS_STARTED)) {
            CanSM_StartTimer(0, CANSM_TIMER_BUS_OFF, CANSM_TIMER_BUS_OFF_SLOW_RECOVERY_MAX);
        }
    }
}

/**
 * @brief Starts a software timer within the CanSM module.
 * @details Initializes a specified timer with a given value and activates it.
 * @param[in] network The ID of the CAN network for which the timer is started.
 * @param[in] timerId The identifier of the timer to start (e.g., CANSM_TIMER_BUS_OFF).
 * @param[in] value The initial value (duration) for the timer.
 */
void CanSM_StartTimer(uint8_t network, CanSM_TimerId_t timerId, uint32_t value) {
    if ((network < CAN_NETWORKS_NO) && (timerId < CANSM_MAX_TIMERS)) {
        CanSM_Timer[network][timerId] = value;
        CanSM_TimerActive[network][timerId] = true;
    }
}

bool CanSM_GetTimer(uint8_t network, CanSM_TimerId_t timerId) {
/**
 * @brief Checks if a specified software timer has expired.
 * @details Returns true if the timer has reached zero and was active. If expired,
 *          the timer is automatically deactivated.
 * @param[in] network The ID of the CAN network.
 * @param[in] timerId The identifier of the timer to check.
 * @return bool True if the timer has expired, false otherwise.
 */
    bool expired = false;
    if ((network < CAN_NETWORKS_NO) && (timerId < CANSM_MAX_TIMERS)) {
        if (CanSM_TimerActive[network][timerId] && (0u == CanSM_Timer[network][timerId])) {
            expired = true;
            CanSM_TimerActive[network][timerId] = false; /* Clear after expiration to prevent re-triggering */
        }
    }
    return expired;
}

/**
 * @brief Initializes the CAN State Manager module.
 * @details Sets the initial state of the CAN network to OFFLINE and clears
 *          all pending events and timers.
 */
void CanSM_Init(void) {
    CanSM_CurrentState = CANSM_STATE_OFFLINE;
    CanSM_RequestedState = CANSM_STATE_OFFLINE;
    CanSM_BusWakeupPending = false;
    CanSM_BusOffPending = false;

    for (uint8_t net = 0; net < CAN_NETWORKS_NO; net++) {
        for (uint8_t t = 0; t < CANSM_MAX_TIMERS; t++) {
            CanSM_TimerActive[net][t] = false;
            CanSM_Timer[net][t] = 0u;
        }
    }
}

/**
 * @brief Requests a state change for the CAN network.
 * @details Allows an upper layer (e.g., Can module) to request a new state
 *          for the CAN network. The actual state transition is handled by CanSM_MainFunction.
 * @param[in] targetState The desired network state.
 * @return Std_ReturnType E_OK if the request was accepted, E_NOT_OK if the module is uninitialized.
 */
Std_ReturnType_t CanSM_RequestState(CanSM_NetworkState_t targetState) {
    Std_ReturnType_t ret_val = E_NOT_OK;
    if (CANSM_STATE_UNINIT != CanSM_CurrentState) {
        /* Cannot request state transitions before initialization */
        ret_val = E_OK;
    }
    CanSM_RequestedState = targetState;
    return ret_val;
}

/**
 * @brief Gets the current state of the CAN network.
 * @details Returns the current operational state of the CAN network.
 * @return CanSM_NetworkState_t The current network state.
 */
CanSM_NetworkState_t CanSM_GetCurrentState(void) {
    return CanSM_CurrentState;
}

/**
 * @brief Reports a bus wakeup event.
 * @details This function is called by lower layers (e.g., CanTrcv) to indicate
 *          that a wakeup event has occurred on the CAN bus.
 */
void CanSM_ReportBusWakeup(void) {
    CanSM_BusWakeupPending = true;
}

/**
 * @brief Reports a Bus-Off event.
 * @details This function is called by lower layers (e.g., CanIf, CanDrv) to indicate
 *          that the CAN controller has entered a Bus-Off state.
 */
void CanSM_ReportBusOff(void) {
    CanSM_BusOffPending = true;
}

/**
 * @brief Main processing function for the CAN State Manager module.
 * @details This function must be called periodically. It handles Bus-Off recovery,
 *          processes pending wakeup events, and manages state transitions based
 *          on requested states.
 */
void CanSM_MainFunction(void) {
    if (CanSM_CurrentState == CANSM_STATE_UNINIT) {
        /* TODO det */
    } else {
        /* Trigger bus off recovery if needed */
        CanSM_BusOffRecovery();

        // 1. Process Hardware Events (Highest Priority)
        if (CanSM_BusOffPending) {
            CanSM_BusOffPending = false;
            CanSM_CurrentState = CANSM_STATE_BUS_OFF;
            CanSM_RequestedState = CANSM_STATE_OFFLINE; // Reset request to force a controlled recovery later
            CanIf_SetControllerMode(0, CANIF_CS_STOPPED); // Assuming Controller 0
            CanSM_StartTimer(0, CANSM_TIMER_BUS_OFF ,CANSM_TIMER_BUS_OFF_FAST_RECOVERY_MAX);

            CAN_BUS_OFF_EVENT();
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
}
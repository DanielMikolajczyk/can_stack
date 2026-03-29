#include "Can.h"
#include "CanIf.h"
#include "CanTp.h"
#include <stddef.h>

#include "CanSM.h"

// Define the max size for a single frame.
// We are assuming CAN-FD capability (64 bytes).
#ifndef CAN_FD_MAX_SINGLE_FRAME_SIZE
#define CAN_FD_MAX_SINGLE_FRAME_SIZE 64U
#endif

void Can_Init(void) {
    // Initialize all sub-modules
    CanSM_Init();
    CanTp_Init();
}

bool Can_RequestState(Can_State_t targetState) {
    CanSM_NetworkState_t smState;
    switch(targetState) {
        case CAN_STATE_ONLINE:  smState = CANSM_STATE_ONLINE; break;
        case CAN_STATE_SLEEP:   smState = CANSM_STATE_SLEEP; break;
        case CAN_STATE_ERROR:   smState = CANSM_STATE_BUS_OFF; break;
        case CAN_STATE_OFFLINE:
        default:                smState = CANSM_STATE_OFFLINE; break;
    }
    return CanSM_RequestState(smState);
}

Can_State_t Can_GetCurrentState(void) {
    CanSM_NetworkState_t smState = CanSM_GetCurrentState();
    switch(smState) {
        case CANSM_STATE_ONLINE:  return CAN_STATE_ONLINE;
        case CANSM_STATE_SLEEP:   return CAN_STATE_SLEEP;
        case CANSM_STATE_BUS_OFF: return CAN_STATE_ERROR;
        case CANSM_STATE_OFFLINE:
        case CANSM_STATE_UNINIT:
        default:                  return CAN_STATE_OFFLINE;
    }
}

bool Can_Write(uint32_t messageId, const uint8_t* payload, uint16_t length) {
    if (payload == NULL || length == 0) {
        return false;
    }

    //TODO: Check if length matches the configuration (?) - drop otherwise (?)

    // Transmission path routing based on payload size
    if (length <= CAN_FD_MAX_SINGLE_FRAME_SIZE) {
        // Fits into a single CAN-FD frame
        return CanIf_Transmit(messageId, payload, length);
    } else {
        // Exceeds single frame size, Transport Protocol is needed
        return CanTp_Transmit(messageId, payload, length);
    }
}

void Can_MainFunction(void) {
    // Process the state machine for network management
    CanSM_MainFunction();
    // Process the transport protocol state machines (for TX and RX)
    CanTp_MainFunction();
}

void Can_RxIndication(const Can_RxPduConfigType *rxConfig, uint32_t canId, CanPduInfoType_t* const canPduInfo){
    // Provide the clean facade API to pass standard CAN messages
    // up to the external application layer.
    // App_OnCanMessageReceived(rxConfig, payload, length);
}
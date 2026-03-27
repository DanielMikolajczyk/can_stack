#include "Can.h"
#include "CanIf.h"
#include "CanTp.h"
#include <stddef.h>

// Define the max size for a single frame.
// We are assuming CAN-FD capability (64 bytes).
#ifndef CAN_MAX_SINGLE_FRAME_SIZE
#define CAN_MAX_SINGLE_FRAME_SIZE 64U
#endif

static Can_State_t currentState = CAN_STATE_OFFLINE;

void Can_Init(void) {
    // Future logic: Initialize CanSM, CanIf, CanTp, etc.
    currentState = CAN_STATE_OFFLINE;
}

bool Can_RequestState(Can_State_t targetState) {
    // Future logic: Route state requests to CanSM
    return true;
}

Can_State_t Can_GetCurrentState(void) {
    return currentState;
}

bool Can_Write(uint32_t messageId, const uint8_t* payload, uint16_t length) {
    if (payload == NULL || length == 0) {
        return false;
    }

    // Transmission path routing based on payload size
    if (length <= CAN_MAX_SINGLE_FRAME_SIZE) {
        // Fits into a single CAN-FD frame
        return CanIf_Transmit(messageId, payload, length);
    } else {
        // Exceeds single frame size, Transport Protocol is needed
        return CanTp_Transmit(messageId, payload, length);
    }
}
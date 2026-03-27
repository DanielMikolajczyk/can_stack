#ifndef CAN_FACADE_H
#define CAN_FACADE_H

#include "CanIf.h"
#include "CanSM.h"
#include "CanTp.h"
#include "CanTrcv.h"

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CAN_STATE_OFFLINE,
    CAN_STATE_ONLINE,
    CAN_STATE_SLEEP,
    CAN_STATE_ERROR
} Can_State_t;

// Initialization and State Management (Handled via CanSM underneath)
void Can_Init(void);
bool Can_RequestState(Can_State_t targetState);
Can_State_t Can_GetCurrentState(void);

// Unified Transmission (Routes to CanIf or CanTp internally)
bool Can_Write(uint32_t messageId, const uint8_t* payload, uint16_t length);

// Main processing function for the entire CAN stack.
void Can_MainFunction(void);

// Callbacks (To be implemented by the outer Application)
extern void App_OnCanMessageReceived(uint32_t messageId, const uint8_t* payload, uint16_t length);
extern void App_OnCanStateChanged(Can_State_t newState);

#endif // CAN_FACADE_H

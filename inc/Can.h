#ifndef CAN_FACADE_H
#define CAN_FACADE_H

#include "Std_Types.h"
#include "CanTypes.h"

// Initialization and State Management (Handled via CanSM underneath)
void Can_Init(void);
bool Can_RequestState(Can_State_t targetState);
Can_State_t Can_GetCurrentState(void);

// Unified Transmission (Routes to CanIf or CanTp internally)
bool Can_Write(uint32_t messageId, const uint8_t* payload, uint16_t length);

// Main processing function for the entire CAN stack.
void Can_MainFunction(void);

// Internal routing callback (called by CanIf for standard messages)
void Can_RxIndication(const Can_RxPduConfigType *rxConfig, uint32_t canId, CanPduInfoType_t* const canPduInfo);

// Callbacks (To be implemented by the outer Application)
extern void App_OnCanMessageReceived(uint32_t messageId, const uint8_t* payload, uint16_t length);
extern void App_OnCanStateChanged(Can_State_t newState);

#endif // CAN_FACADE_H

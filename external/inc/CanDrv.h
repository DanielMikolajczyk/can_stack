#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include "CanTypes.h"

void CAN_Mailbox0_Interrupt_Handler(void);

void CanDriver_Transmit(const Can_TxPduConfigType* txConfig, CanPdu_t* canPdu, uint32_t dlc);
Std_ReturnType_t CanDriver_SetControllerMode(uint8_t controllerId, CanIf_ControllerMode_t controllerMode);

#endif /* CAN_DRIVER_H */

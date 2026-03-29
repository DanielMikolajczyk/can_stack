#ifndef CAN_DRIVER_HEADER_GUARD
#define CAN_DRIVER_HEADER_GUARD

#include "CanTypes.h"

void CAN_Mailbox0_Interrupt_Handler(void);

void CanDriver_Transmit(const Can_TxPduConfigType* txConfig, CanPdu_t canPdu);
#endif /* CAN_DRIVER_HEADER_GUARD */

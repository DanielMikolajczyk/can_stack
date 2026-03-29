#ifndef CANIF_H
#define CANIF_H

#include "Std_Types.h"
#include "CanTypes.h"

// Interface layer transmission
// Handles standard CAN/CAN-FD frames up to 64 bytes
Std_ReturnType_t CanIf_Transmit(uint32_t messageId, const uint8_t* payload, uint16_t length);

// Called by the hardware driver (e.g. from an ISR) when a frame is received.
// Routes the frame upward based on payload size/type.
void CanIf_RxIndication(const CanIf_HwType_t* const mailboxInfo, CanPdu_t* const canPdu);

#endif // CANIF_H
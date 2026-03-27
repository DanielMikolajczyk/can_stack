#ifndef CANIF_H
#define CANIF_H

#include <stdint.h>
#include <stdbool.h>

// Interface layer transmission
// Handles standard CAN/CAN-FD frames up to 64 bytes
bool CanIf_Transmit(uint32_t messageId, const uint8_t* payload, uint16_t length);

// Called by the hardware driver (e.g. from an ISR) when a frame is received.
// Routes the frame upward based on payload size/type.
void CanIf_RxIndication(uint32_t messageId, uint32_t dlc, bool isCanFd, const uint8_t* payload);

#endif // CANIF_H
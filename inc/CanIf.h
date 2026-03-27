#ifndef CANIF_H
#define CANIF_H

#include <stdint.h>
#include <stdbool.h>

// Interface layer transmission
// Handles standard CAN/CAN-FD frames up to 64 bytes
bool CanIf_Transmit(uint32_t messageId, const uint8_t* payload, uint16_t length);

#endif // CANIF_H
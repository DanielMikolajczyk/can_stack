#ifndef CANTP_H
#define CANTP_H

#include <stdint.h>
#include <stdbool.h>

// Transport protocol transmission
// Handles large payload segmentation (ISO-TP)
bool CanTp_Transmit(uint32_t messageId, const uint8_t* payload, uint16_t length);

#endif // CANTP_H
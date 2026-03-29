#ifndef CANTP_H
#define CANTP_H

#include "Std_Types.h"
#include "CanIf.h"
#include "Can_cfg.h"

// Initialize the CanTp module and its internal states.
void CanTp_Init(void);

// Transport protocol transmission
// Handles large payload segmentation (ISO-TP)
bool CanTp_Transmit(uint32_t messageId, const uint8_t* payload, uint16_t length);

// Main processing function for CanTp, drives the state machines.
// This should be called periodically from the main application loop.
void CanTp_MainFunction(void);

// Called by CanIf when an ISO-TP segment is received.
void CanTp_RxIndication(const Can_RxPduConfigType *rxConfig, CanPdu_t* const canPdu);

#endif // CANTP_H
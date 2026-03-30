#ifndef CANIF_H
#define CANIF_H

#include "Std_Types.h"
#include "CanTypes.h"

typedef enum {
    CANIF_CS_UNINIT = 0,
    CANIF_CS_SLEEP,
    CANIF_CS_STARTED,
    CANIF_CS_STOPPED
} CanIf_ControllerMode_t;

Std_ReturnType_t CanIf_Transmit(const Can_TxPduConfigType* txConfig, CanPdu_t* canPdu);
void CanIf_RxIndication(const CanIf_HwType_t* const mailboxInfo, CanPdu_t* const canPdu);

// TODO: move to utils
Std_ReturnType_t CanIf_FindTxCanFrameConfig(const Can_TxPduConfigType** txConfig, uint32_t canId);

Std_ReturnType_t CanIf_SetControllerMode(uint8_t controllerId, CanIf_ControllerMode_t controllerMode);

#endif /* CANIF_H */
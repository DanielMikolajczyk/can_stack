#ifndef CANIF_H
#define CANIF_H

#include "Std_Types.h"
#include "CanTypes.h"

Std_ReturnType_t CanIf_Transmit(const Can_TxPduConfigType* txConfig, CanPdu_t* canPdu);
void CanIf_RxIndication(const CanIf_HwType_t* const mailboxInfo, CanPdu_t* const canPdu);

// TODO: move to utils
Std_ReturnType_t CanIf_FindTxCanFrameConfig(const Can_TxPduConfigType** txConfig, uint32_t canId);

#endif /* CANIF_H */
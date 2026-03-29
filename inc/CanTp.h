#ifndef CANTP_H
#define CANTP_H

#include "Std_Types.h"
#include "CanIf.h"
#include "Can_cfg.h"

void CanTp_Init(void);
void CanTp_MainFunction(void);
Std_ReturnType_t CanTp_Transmit(const Can_TxPduConfigType* txConfig, CanPdu_t* canPdu);
void CanTp_RxIndication(const Can_RxPduConfigType *rxConfig, CanPdu_t* const canPdu);

#endif // CANTP_H
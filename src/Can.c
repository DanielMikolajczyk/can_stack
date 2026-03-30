#include "Can.h"
#include "CanIf.h"
#include "CanTp.h"
#include <stddef.h>

#include "CanSM.h"
#include "CanCfg.h"

void Can_Init(void) {
    CanSM_Init();
    CanTp_Init();
}

Std_ReturnType_t Can_RequestState(Can_State_t targetState) {
    CanSM_NetworkState_t smState;
    switch(targetState) {
        case CAN_STATE_ONLINE:  smState = CANSM_STATE_ONLINE; break;
        case CAN_STATE_SLEEP:   smState = CANSM_STATE_SLEEP; break;
        case CAN_STATE_ERROR:   smState = CANSM_STATE_BUS_OFF; break;
        case CAN_STATE_OFFLINE:
        default:                smState = CANSM_STATE_OFFLINE; break;
    }
    return CanSM_RequestState(smState);
}

Can_State_t Can_GetCurrentState(void) {
    CanSM_NetworkState_t smState = CanSM_GetCurrentState();
    switch(smState) {
        case CANSM_STATE_ONLINE:  return CAN_STATE_ONLINE;
        case CANSM_STATE_SLEEP:   return CAN_STATE_SLEEP;
        case CANSM_STATE_BUS_OFF: return CAN_STATE_ERROR;
        case CANSM_STATE_OFFLINE:
        case CANSM_STATE_UNINIT:
        default:                  return CAN_STATE_OFFLINE;
    }
}

Std_ReturnType_t Can_Write(uint32_t canId, const uint8_t* payload, uint16_t length) {
    const Can_TxPduConfigType* txConfig = NULL_PTR;
    Std_ReturnType_t ret_val = E_OK;
    CanPdu_t canPdu = { 0u };

    /* Error checks */
    if ((NULL_PTR == payload) || (0u == length)) {
        ;//TODO: det
        ret_val = E_NOT_OK;
    }

    if ((E_NOT_OK == ret_val) || (E_NOT_OK == CanIf_FindTxCanFrameConfig(&txConfig, canId))) {
        ;//TODO: det ID Not found
        ret_val = E_NOT_OK;
    }

    if ((E_NOT_OK == ret_val) || (length != txConfig->length)) {
        ;//TODO: det length doesn't match
        ret_val = E_NOT_OK;
    }

    /* Implementation */
    if (E_OK == ret_val) {
        canPdu.sduLength = length;
        canPdu.sduDataPtr = (uint8_t*)payload;

        /* Transmission path routing based on payload size */
        if (
            ((CAN_FRAME_CLASSIC == txConfig->frameType) && (CAN_MAX_SINGLE_FRAME_SIZE >= txConfig->length)) ||
            ((CAN_FRAME_FD == txConfig->frameType) && (CAN_FD_MAX_SINGLE_FRAME_SIZE >= txConfig->length))
        ) {
            /* Fits into a single frame */
            CanIf_Transmit(txConfig, &canPdu);
        } else {
            /* Exceeds single frame size, Transport Protocol is needed */
            (void)CanTp_Transmit(txConfig, &canPdu);
        }
    }
    return ret_val;
}

void Can_MainFunction(void) {
    CanSM_MainFunction();
    CanTp_MainFunction();
}

#include <stdio.h>
void Can_RxIndication(const Can_RxPduConfigType *rxConfig, CanPdu_t* const canPdu){
    // Provide the clean facade API to pass standard CAN messages
    // up to the external application layer.
    // App_OnCanMessageReceived(rxConfig, payload, length);
    printf("frame received\n");
}
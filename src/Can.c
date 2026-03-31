#include "Can.h"
#include "CanIf.h"
#include "CanTp.h"
#include <stddef.h>

#include "CanSM.h"
#include "CanCfg.h"

/**
 * @brief Initializes the CAN stack.
 * @details This function initializes all sub-modules of the CAN stack, including
 *          the CAN State Manager (CanSM) and the CAN Transport Protocol (CanTp).
 *          It must be called once at system startup before any other CAN stack
 *          function is used.
 */
void Can_Init(void) {
    CanSM_Init();
    CanTp_Init();
}

/**
 * @brief Requests a change in the network state.
 * @details This function translates a generic application-level state request
 *          (e.g., ONLINE, SLEEP) into a specific state for the CanSM module.
 * @param[in] targetState The desired network state from the application's perspective.
 * @return Std_ReturnType E_OK if the request was accepted, E_NOT_OK otherwise.
 */
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

/**
 * @brief Gets the current state of the CAN network.
 * @details This function retrieves the underlying state from the CanSM and maps it
 *          to a generic application-level state.
 * @return Can_State_t The current network state.
 */
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

/**
 * @brief Transmits a CAN message.
 * @details This is the main entry point for the application to send data. It performs
 *          initial validation, finds the message configuration, and routes the PDU
 *          to either CanIf (for single-frame) or CanTp (for multi-frame) based
 *          on the static configuration.
 * @param[in] canId The CAN identifier of the message to be sent.
 * @param[in] payload Pointer to the data payload.
 * @param[in] length The length of the payload in bytes.
 * @return Std_ReturnType E_OK if the transmission request was successfully initiated,
 *         E_NOT_OK if an error occurred (e.g., invalid parameters, network offline).
 */
Std_ReturnType_t Can_Write(uint32_t canId, const uint8_t* payload, uint16_t length) {
    const Can_TxPduConfigType* txConfig = NULL_PTR;
    Std_ReturnType_t ret_val = E_OK;
    CanPdu_t canPdu = { 0u };

    /* Error checks */
    if ((NULL_PTR == payload) || (0u == length)) {
        ;//TODO: det
        ret_val = E_NOT_OK;
    }

    if ((E_NOT_OK == ret_val) || (CAN_STATE_ONLINE != Can_GetCurrentState())) {
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

/**
 * @brief Main processing function for the CAN stack.
 * @details This function must be called periodically from the application's main loop.
 *          It drives the state machines of the CanSM and CanTp modules, handling
 *          network state transitions, Bus-Off recovery, and transport protocol
 *          segmentation/reassembly.
 */
void Can_MainFunction(void) {
    CanSM_MainFunction();
    CanTp_MainFunction();
}

#include <stdio.h>
/**
 * @brief Internal callback for standard message reception.
 * @details This function is called by the CanIf module when a complete, standard
 *          (non-TP) CAN frame has been received. It forwards the message to the
 *          application layer via the App_OnCanMessageReceived callback.
 * @param[in] rxConfig Pointer to the configuration of the received PDU.
 * @param[in] canPdu Pointer to the PDU containing the received data.
 */
void Can_RxIndication(const Can_RxPduConfigType *rxConfig, CanPdu_t* const canPdu){
    // Provide the clean facade API to pass standard CAN messages
    // up to the external application layer.
    // App_OnCanMessageReceived(rxConfig, payload, length);
    printf("frame received\n");
}
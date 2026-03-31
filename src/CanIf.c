#include "Std_Types.h"
#include "CanCfg.h"
#include "CanTypes.h"
#include "CanIf.h"
#include "CanTp.h"
#include "Can.h" // For routing standard messages up the stack
#include "CanDrv.h" // For routing standard messages up the stack

// ============================================================================
// Mock Hardware Driver Layer (e.g., STM32 FDCAN HAL)
// Normally, this would be included via a driver header like "stm32h7xx_hal_fdcan.h"
// ============================================================================

typedef enum {
    HAL_OK      = 0x00U,
    HAL_ERROR   = 0x01U,
    HAL_BUSY    = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

typedef struct {
    uint32_t Identifier;    // CAN ID
    uint32_t IdType;        // Standard or Extended ID
    uint32_t TxFrameType;   // Data or Remote frame
    uint32_t DataLength;    // DLC (Data Length Code)
    uint32_t FDFormat;      // Classic CAN or CAN-FD
    uint32_t BitRateSwitch; // BRS enabled/disabled
} FDCAN_TxHeaderTypeDef;

#define FDCAN_STANDARD_ID  (0x00000000U)
#define FDCAN_EXTENDED_ID  (0x40000000U)
#define FDCAN_DATA_FRAME   (0x00000000U)
#define FDCAN_FD_CAN       (0x00210000U)
#define FDCAN_CLASSIC_CAN  (0x00000000U)
#define FDCAN_BRS_ON       (0x00000001U)
#define FDCAN_BRS_OFF      (0x00000000U)

// ============================================================================
// CanIf Implementation
// ============================================================================

// Helper: Convert byte length to CAN-FD DLC code
static uint32_t CanIf_LengthToDLC(uint16_t length) {
    if (length <= 8)  return length; // 0-8 map directly to DLC 0-8
    if (length <= 12) return 9;
    if (length <= 16) return 10;
    if (length <= 20) return 11;
    if (length <= 24) return 12;
    if (length <= 32) return 13;
    if (length <= 48) return 14;
    return 15;                       // Up to 64 bytes
}

// // Helper: Convert CAN-FD DLC code back to byte length
// static uint16_t CanIf_DLCToLength(uint32_t dlc, Can_FrameType type) {
//     // Classic CAN limits length to 8 bytes maximum, even if DLC is higher
//     if ((CAN_FRAME_FD == type) && (dlc > 8u)) return 8u;

//     if (dlc <= 8u) return (uint16_t)dlc;
//     switch (dlc) {
//         case 9u: return 12u;
//         case 10u: return 16u;
//         case 11u: return 20u;
//         case 12u: return 24u;
//         case 13u: return 32u;
//         case 14u: return 48u;
//         case 15u: return 64u;
//         default: return 8u;
//     }
// }

//Move to Utils
Std_ReturnType_t CanIf_FindTxCanFrameConfig(const Can_TxPduConfigType** txConfig, uint32_t canId) {
    Std_ReturnType_t ret_val = E_NOT_OK;

    //TODO multuple TX pdu config for each hoh
    for (uint16_t i = 0u ; i < CAN_NUM_TX_PDUS; i++) {
        // TODO - split not by array but different accesses
        if (canId == CanConfig.TxPduConfig[i].canId) {
            *txConfig = &(CanConfig.TxPduConfig[i]);
            ret_val = E_OK;
            break;
        }
    }

    return ret_val;
}

static Std_ReturnType_t CanIf_FindRxCanFrameConfig(const Can_RxPduConfigType** rxConfig, uint8_t hoh, uint32_t canId) {
    Std_ReturnType_t ret_val = E_NOT_OK;

    //TODO multuple RX pdu config for each hoh
    for (uint16_t i = 0u ; i < CAN_NUM_RX_PDUS; i++) {
        // TODO - split not by array but different accesses
        if (canId == CanConfig.RxPduConfig[i].canId) {
            *rxConfig = &(CanConfig.RxPduConfig[i]);
            ret_val = E_OK;
            break;
        }
    }

    return ret_val;
}

Std_ReturnType_t CanIf_Transmit(const Can_TxPduConfigType* txConfig, CanPdu_t* canPdu){
    Std_ReturnType_t ret_val = E_OK;
    uint32_t dlc;

    if ((NULL_PTR == canPdu->sduDataPtr) || (0u == canPdu->sduLength) || (canPdu->sduLength > 64u)) {
        // TODO:Det
        ret_val = E_NOT_OK;
    }

    /* Do not transmit if bus off occured (TODO: verify correctness of the solution) */
    if ((E_NOT_OK == ret_val) || (CAN_STATE_ONLINE != Can_GetCurrentState())) {
        ret_val = E_NOT_OK;
    }

    if (E_OK == ret_val) {
        dlc = CanIf_LengthToDLC(canPdu->sduLength);
        CanDriver_Transmit(txConfig, canPdu, dlc);
    }

    return ret_val;
}

void CanIf_RxIndication(const CanIf_HwType_t* const mailboxInfo, CanPdu_t* const canPdu) {
    const Can_RxPduConfigType* rxConfig = NULL_PTR;
    const uint32_t canId = mailboxInfo->canId;
    uint16_t length = 0u;

    if (E_NOT_OK == CanIf_FindRxCanFrameConfig(&rxConfig, mailboxInfo->hoh, mailboxInfo->canId)) {
        //TODO: Det error
    } else {

        //TODO: Check deffered or instant
        if (rxConfig->protocol == CAN_TP) {
            CanTp_RxIndication(rxConfig, canPdu);
        } else {
            Can_RxIndication(rxConfig, canPdu);
        }
    }
}

Std_ReturnType_t CanIf_SetControllerMode(uint8_t controllerId, CanIf_ControllerMode_t controllerMode) {
    /* Propagate the mode change down to the hardware driver */
    return CanDriver_SetControllerMode(controllerId, controllerMode);
}

//TODO: remove from here
Std_ReturnType_t CanDriver_SetControllerMode(uint8_t controllerId, CanIf_ControllerMode_t controllerMode) {
    // TODO: Write to hardware registers (e.g. clear Init/Sleep bits)
    printf("CanDriver: Controller %d mode set to %d\n", controllerId, controllerMode);
    return E_OK;
}
#include "CanIf.h"
#include <stddef.h>
#include "CanTp.h"
#include "Can.h" // For routing standard messages up the stack

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

// Mock transmission function
static HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_TxHeaderTypeDef *pTxHeader, const uint8_t *pTxData) {
    // In a real environment, this copies the header and data into the FDCAN peripheral's TX FIFO.
    (void)pTxHeader;
    (void)pTxData;
    return HAL_OK;
}

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

bool CanIf_Transmit(uint32_t messageId, const uint8_t* payload, uint16_t length) {
    if (payload == NULL || length == 0 || length > 64) {
        return false;
    }

    FDCAN_TxHeaderTypeDef txHeader;

    // Determine if the ID is Standard (11-bit) or Extended (29-bit)
    // Standard IDs max out at 0x7FF
    if (messageId > 0x7FF) {
        txHeader.Identifier = messageId & 0x1FFFFFFF;
        txHeader.IdType = FDCAN_EXTENDED_ID;
    } else {
        txHeader.Identifier = messageId & 0x7FF;
        txHeader.IdType = FDCAN_STANDARD_ID;
    }

    txHeader.TxFrameType = FDCAN_DATA_FRAME;
    txHeader.DataLength = CanIf_LengthToDLC(length);

    // Assume CAN-FD format with Bit Rate Switching (BRS) for anything larger than 8 bytes
    if (length > 8) {
        txHeader.FDFormat = FDCAN_FD_CAN;
        txHeader.BitRateSwitch = FDCAN_BRS_ON;
    } else {
        txHeader.FDFormat = FDCAN_CLASSIC_CAN;
        txHeader.BitRateSwitch = FDCAN_BRS_OFF;
    }

    // Pass to hardware driver
    HAL_StatusTypeDef status = HAL_FDCAN_AddMessageToTxFifoQ(&txHeader, payload);

    return (status == HAL_OK);
}

// ============================================================================
// Reception Path
// ============================================================================

// Helper: Convert CAN-FD DLC code back to byte length
static uint16_t CanIf_DLCToLength(uint32_t dlc, bool isCanFd) {
    // Classic CAN limits length to 8 bytes maximum, even if DLC is higher
    if (!isCanFd && dlc > 8) return 8;

    if (dlc <= 8) return (uint16_t)dlc;
    switch (dlc) {
        case 9: return 12;
        case 10: return 16;
        case 11: return 20;
        case 12: return 24;
        case 13: return 32;
        case 14: return 48;
        case 15: return 64;
        default: return 8;
    }
}

// Entry point for the Hardware Driver RX Interrupt
void CanIf_RxIndication(uint32_t messageId, uint32_t dlc, bool isCanFd, const uint8_t* payload) {
    if (payload == NULL) return;

    uint16_t length = CanIf_DLCToLength(dlc, isCanFd);

    // Routing heuristic:
    // Typical ISO-TP Diagnostic IDs range from 0x7E0 to 0x7EF.
    // TODO: lookup on static configuration table
    // In a production system, this would be a lookup in a static configuration table.
    bool isTpMessage = (messageId >= 0x7E0 && messageId <= 0x7EF);

    if (isTpMessage) {
        CanTp_RxIndication(messageId, payload, length);
    } else {
        Can_RxIndication(messageId, payload, length);
    }
}
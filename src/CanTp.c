#include "CanTp.h"
#include "CanIf.h"
#include <stddef.h>
#include <string.h>
#include "Can.h" // For routing reassembled messages up to the Facade
#include "CanIf.h"
#include "Can_cfg.h"

// ISO 15765-2 Protocol Control Information (PCI) types
#define PCI_TYPE_SINGLE_FRAME       0x00
#define PCI_TYPE_FIRST_FRAME        0x10
#define PCI_TYPE_CONSECUTIVE_FRAME  0x20
#define PCI_TYPE_FLOW_CONTROL       0x30

// Maximum number of concurrent TP transmissions.
#ifndef CANTP_MAX_TX_CHANNELS
#define CANTP_MAX_TX_CHANNELS 2
#endif

// Maximum number of concurrent TP receptions.
#ifndef CANTP_MAX_RX_CHANNELS
#define CANTP_MAX_RX_CHANNELS 2
#endif

// Maximum payload size for reassembling a received segmented message
#ifndef CANTP_MAX_RX_PAYLOAD
#define CANTP_MAX_RX_PAYLOAD 256
#endif

// Max payload for a First Frame (8-byte CAN frame: 2 for PCI, 6 for data)
#define CANTP_FF_MAX_PAYLOAD 6

// Max payload for a Consecutive Frame (8-byte CAN frame: 1 for PCI, 7 for data)
#define CANTP_CF_MAX_PAYLOAD 7

typedef enum {
    CANTP_TX_STATE_IDLE,
    CANTP_TX_STATE_SEND_FF,
    CANTP_TX_STATE_WAIT_FC,
    CANTP_TX_STATE_SEND_CF
} CanTp_TxState_t;

typedef struct {
    CanTp_TxState_t state;
    uint32_t messageId;
    const uint8_t* data;
    uint16_t totalSize;
    uint16_t remainingSize;
    uint8_t sequenceNumber;

    // Flow Control parameters (to be received from peer)
    uint8_t blockSize;      // Number of CFs to send before next FC
    uint8_t cfSentInBlock;  // Counter for CFs sent in the current block
} CanTp_TxChannel_t;

typedef enum {
    CANTP_RX_STATE_IDLE,
    CANTP_RX_STATE_RECEIVING_CF
} CanTp_RxState_t;

typedef struct {
    CanTp_RxState_t state;
    uint32_t messageId;
    uint8_t buffer[CANTP_MAX_RX_PAYLOAD];
    uint16_t totalSize;
    uint16_t receivedSize;
    uint8_t expectedSequenceNumber;
} CanTp_RxChannel_t;

static CanTp_TxChannel_t txChannels[CANTP_MAX_TX_CHANNELS];
static CanTp_RxChannel_t rxChannels[CANTP_MAX_RX_CHANNELS];

// Forward declarations for internal state processing functions
static void CanTp_ProcessSendFF(CanTp_TxChannel_t* channel);
static void CanTp_ProcessSendCF(CanTp_TxChannel_t* channel);

void CanTp_Init(void) {
    for (int i = 0; i < CANTP_MAX_TX_CHANNELS; ++i) {
        txChannels[i].state = CANTP_TX_STATE_IDLE;
    }
    for (int i = 0; i < CANTP_MAX_RX_CHANNELS; ++i) {
        rxChannels[i].state = CANTP_RX_STATE_IDLE;
    }
}

bool CanTp_Transmit(uint32_t messageId, const uint8_t* payload, uint16_t length) {
    if (payload == NULL || length == 0) {
        return false;
    }

    // Find an available (idle) transmission channel
    for (int i = 0; i < CANTP_MAX_TX_CHANNELS; ++i) {
        if (txChannels[i].state == CANTP_TX_STATE_IDLE) {
            CanTp_TxChannel_t* channel = &txChannels[i];
            channel->messageId = messageId;
            channel->data = payload;
            channel->totalSize = length;
            channel->remainingSize = length;
            channel->sequenceNumber = 1; // First CF will have sequence number 1
            channel->blockSize = 0;      // Default: send all frames without waiting for FC
            channel->cfSentInBlock = 0;

            // Kick off the state machine; MainFunction will handle the rest
            channel->state = CANTP_TX_STATE_SEND_FF;
            return true;
        }
    }

    // No channel available
    return false;
}

void CanTp_MainFunction(void) {
    for (int i = 0; i < CANTP_MAX_TX_CHANNELS; ++i) {
        CanTp_TxChannel_t* channel = &txChannels[i];

        switch (channel->state) {
            case CANTP_TX_STATE_SEND_FF:
                CanTp_ProcessSendFF(channel);
                break;

            case CANTP_TX_STATE_WAIT_FC:
                // We are waiting for an actual Flow Control frame, handled in CanTp_RxIndication.
                // In a production system, a timeout check would be implemented here.
                // TODO: timeout check
                break;

            case CANTP_TX_STATE_SEND_CF:
                CanTp_ProcessSendCF(channel);
                break;

            case CANTP_TX_STATE_IDLE:
            default:
                // Do nothing for idle channels
                break;
        }
    }
}

static void CanTp_ProcessSendFF(CanTp_TxChannel_t* channel) {
    uint8_t frame_payload[8];

    // PCI: 0x1 (FF) and 12-bit length
    frame_payload[0] = PCI_TYPE_FIRST_FRAME | ((channel->totalSize >> 8) & 0x0F);
    frame_payload[1] = channel->totalSize & 0xFF;
    //TODO: Can-FD specific shouldn't have 0x00 0x00 4bytes of size?

    // Copy first part of the data
    memcpy(&frame_payload[2], channel->data, CANTP_FF_MAX_PAYLOAD);

    if (CanIf_Transmit(channel->messageId, frame_payload, 8)) {
        // Update channel state on successful transmission
        channel->data += CANTP_FF_MAX_PAYLOAD;
        channel->remainingSize -= CANTP_FF_MAX_PAYLOAD;
        channel->state = CANTP_TX_STATE_WAIT_FC;
    }
    // If transmit fails, we will retry on the next CanTp_MainFunction call.
}

void CanTp_RxIndication(const Can_RxPduConfigType *rxConfig, uint32_t canId, CanPduInfoType_t* const canPduInfo) {
    if (canPduInfo->sduDataPtr == NULL || canPduInfo->sduLength == 0) return;

    uint8_t pciType = canPduInfo->sduDataPtr[0] & 0xF0;

    switch (pciType) {
        case PCI_TYPE_FLOW_CONTROL: {
            uint8_t flowStatus = canPduInfo->sduDataPtr[0] & 0x0F;
            for (int i = 0; i < CANTP_MAX_TX_CHANNELS; ++i) {
                if (txChannels[i].state == CANTP_TX_STATE_WAIT_FC) {
                    if (flowStatus == 0) { // Clear To Send (CTS)
                        txChannels[i].blockSize = canPduInfo->sduDataPtr[1];
                        txChannels[i].cfSentInBlock = 0;
                        txChannels[i].state = CANTP_TX_STATE_SEND_CF;
                    } else if (flowStatus == 2) { // Overflow (Receiver buffer full)
                        txChannels[i].state = CANTP_TX_STATE_IDLE; // Abort
                    }
                    // If flowStatus == 1 (Wait), we stay in WAIT_FC
                    break;
                }
            }
            break;
        }
        case PCI_TYPE_SINGLE_FRAME: {
            uint8_t sfLen = canPduInfo->sduDataPtr[0] & 0x0F;
            uint8_t dataOffset = 1;

            // Handle CAN-FD extended Single Frame length
            if (sfLen == 0 && canPduInfo->sduLength > 8) {
                sfLen = canPduInfo->sduDataPtr[1];
                dataOffset = 2;
            }

            Can_RxIndication(rxConfig, canId, canPduInfo);
            break;
        }
        case PCI_TYPE_FIRST_FRAME: {
            uint16_t ffLen = ((canPduInfo->sduDataPtr[0] & 0x0F) << 8) | canPduInfo->sduDataPtr[1];
            uint8_t dataOffset = 2;

            // Handle CAN-FD extended First Frame length
            if (ffLen == 0 && canPduInfo->sduLength > 8) {
                ffLen = (canPduInfo->sduDataPtr[2] << 24) | (canPduInfo->sduDataPtr[3] << 16) | (canPduInfo->sduDataPtr[4] << 8) | canPduInfo->sduDataPtr[5];
                dataOffset = 6;
            }

            if (ffLen > CANTP_MAX_RX_PAYLOAD) {
                return; // Payload exceeds our buffer size, drop or reply with FC Overflow
            }

            for (int i = 0; i < CANTP_MAX_RX_CHANNELS; ++i) {
                if (rxChannels[i].state == CANTP_RX_STATE_IDLE) {
                    rxChannels[i].state = CANTP_RX_STATE_RECEIVING_CF;
                    rxChannels[i].messageId = canId;
                    rxChannels[i].totalSize = ffLen;

                    uint8_t copyLen = canPduInfo->sduLength - dataOffset;
                    memcpy(rxChannels[i].buffer, &canPduInfo->sduDataPtr[dataOffset], copyLen);
                    rxChannels[i].receivedSize = copyLen;
                    rxChannels[i].expectedSequenceNumber = 1; // Next CF must be seq 1

                    // Send Flow Control (CTS) back to peer
                    uint8_t fcFrame[3] = { PCI_TYPE_FLOW_CONTROL | 0x00, 0x00, 0x00 }; // CTS, BS=0, STmin=0
                    // Note: In automotive setups, request IDs respond on +8 (e.g., 0x7E0 answers on 0x7E8).
                    uint32_t responseId = (canId <= 0x7E7) ? canId + 8 : canId;
                    CanIf_Transmit(responseId, fcFrame, 3);
                    break;
                }
            }
            break;
        }
        case PCI_TYPE_CONSECUTIVE_FRAME: {
            uint8_t seqNum = canPduInfo->sduDataPtr[0] & 0x0F;
            for (int i = 0; i < CANTP_MAX_RX_CHANNELS; ++i) {
                if (rxChannels[i].state == CANTP_RX_STATE_RECEIVING_CF) {
                    // Note: For a strict implementation, we should also match rxChannels[i].messageId
                    if (seqNum != rxChannels[i].expectedSequenceNumber) {
                        rxChannels[i].state = CANTP_RX_STATE_IDLE; // Sequence error, abort
                        break;
                    }

                    uint8_t copyLen = canPduInfo->sduLength - 1;
                    // Protect against overflowing the requested total size
                    if (rxChannels[i].receivedSize + copyLen > rxChannels[i].totalSize) {
                        copyLen = rxChannels[i].totalSize - rxChannels[i].receivedSize;
                    }

                    memcpy(&rxChannels[i].buffer[rxChannels[i].receivedSize], &canPduInfo->sduDataPtr[1], copyLen);
                    rxChannels[i].receivedSize += copyLen;
                    rxChannels[i].expectedSequenceNumber = (rxChannels[i].expectedSequenceNumber + 1) & 0x0F;

                    // If all bytes received, pass up to the application and free channel
                    if (rxChannels[i].receivedSize >= rxChannels[i].totalSize) {
                        Can_RxIndication(rxConfig, canId, canPduInfo);
                        rxChannels[i].state = CANTP_RX_STATE_IDLE;
                    }
                    break;
                }
            }
            break;
        }
    }
}

static void CanTp_ProcessSendCF(CanTp_TxChannel_t* channel) {
    if (channel->remainingSize == 0) {
        channel->state = CANTP_TX_STATE_IDLE; // Transmission complete
        return;
    }

    // If blockSize is > 0, check if we've sent the whole block
    if (channel->blockSize > 0 && channel->cfSentInBlock >= channel->blockSize) {
        channel->state = CANTP_TX_STATE_WAIT_FC; // Wait for the next FC
        return;
    }

    uint8_t frame_payload[8];
    uint8_t bytes_to_send = (channel->remainingSize > CANTP_CF_MAX_PAYLOAD) ? CANTP_CF_MAX_PAYLOAD : channel->remainingSize;

    // PCI: 0x2 (CF) and 4-bit sequence number
    frame_payload[0] = PCI_TYPE_CONSECUTIVE_FRAME | (channel->sequenceNumber & 0x0F);

    // Copy the next chunk of data
    memcpy(&frame_payload[1], channel->data, bytes_to_send);

    // The total length of the CAN frame is 1 (PCI) + data bytes
    uint8_t frame_dlc = 1 + bytes_to_send;

    if (CanIf_Transmit(channel->messageId, frame_payload, frame_dlc)) {
        // Update channel state on successful transmission
        channel->data += bytes_to_send;
        channel->remainingSize -= bytes_to_send;
        channel->sequenceNumber = (channel->sequenceNumber + 1) & 0x0F;
        channel->cfSentInBlock++;

        if (channel->remainingSize == 0) {
            channel->state = CANTP_TX_STATE_IDLE; // Transmission finished
        }
    }
    // If transmit fails, we will retry on the next CanTp_MainFunction call.
}
#include "CanTp.h"
#include "CanIf.h"
#include <stddef.h>
#include <string.h>

// ISO 15765-2 Protocol Control Information (PCI) types
#define PCI_TYPE_SINGLE_FRAME       0x00
#define PCI_TYPE_FIRST_FRAME        0x10
#define PCI_TYPE_CONSECUTIVE_FRAME  0x20
#define PCI_TYPE_FLOW_CONTROL       0x30

// Maximum number of concurrent TP transmissions.
#ifndef CANTP_MAX_TX_CHANNELS
#define CANTP_MAX_TX_CHANNELS 2
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

static CanTp_TxChannel_t txChannels[CANTP_MAX_TX_CHANNELS];

// Forward declarations for internal state processing functions
static void CanTp_ProcessSendFF(CanTp_TxChannel_t* channel);
static void CanTp_ProcessSendCF(CanTp_TxChannel_t* channel);

void CanTp_Init(void) {
    for (int i = 0; i < CANTP_MAX_TX_CHANNELS; ++i) {
        txChannels[i].state = CANTP_TX_STATE_IDLE;
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
                // --- SIMULATION HOOK ---
                // In a real implementation, we would wait for an FC frame from the receiver.
                // For now, we assume we received a "Continue To Send" (CTS) FC frame
                // that allows us to send all remaining frames without further flow control.
                channel->blockSize = 0; // A block size of 0 means send all CFs.
                channel->cfSentInBlock = 0;
                channel->state = CANTP_TX_STATE_SEND_CF;
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
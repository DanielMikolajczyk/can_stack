#include <stdio.h>
#include "Can.h"
#include "CanTypes.h"
#include "CanIf.h"
#include "CanDrv.h"

static bool CF_Received = false;

// Application callback when a full CAN message or reassembled TP message arrives
void App_OnCanMessageReceived(uint32_t messageId, const uint8_t* payload, uint16_t length) {
    printf("App received MSG ID: 0x%X | Length: %d\n", messageId, length);
}

// Application callback when network state changes
void App_OnCanStateChanged(Can_State_t newState) {
    printf("App Network State Changed: %d\n", newState);
}

void CanDriver_Transmit(const Can_TxPduConfigType* txConfig, CanPdu_t* canPdu, uint32_t dlc){
    CF_Received = true;
    printf("CAN Driver Transmit: ID: %#x, Length %d\n", txConfig->canId, canPdu->sduLength);
}

static void simple_rx_frame(void) {
    uint8_t payload[8] = {
        1,2,3,4,5,6,7,8
    };

    const CanIf_HwType_t mailboxInfo = {
        .canId = 0x18FF0100UL,
        .hoh = 0,
        .controllerId = 0,
    };
    CanPdu_t canPdu = {
        .sduLength = 8,
        .sduDataPtr = payload,
    };
    CanIf_RxIndication(&mailboxInfo, &canPdu);
}

static void tp_rx_frame(void) {
    uint8_t ff_payload[64] = {
        // FirstFrame
        0x10,
        //Size
        0,0,0,0,184,
        7,8,9,10,11,12,13,14,15,16,
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
    };

    uint8_t cf_payload[64] = {
        // ConsecutiveFrame + Index
        0x21,
        //Index
        0,0,0,0,184,
        7,8,9,10,11,12,13,14,15,16,
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
    };

    const CanIf_HwType_t mailboxInfo = {
        .canId = 0x18FF0102UL,
        .hoh = 0,
        .controllerId = 0,
    };

    CanPdu_t canPdu = {
        .sduLength = 64,
        .sduDataPtr = ff_payload,
    };

    // --------------------TEST--------------------
    CanIf_RxIndication(&mailboxInfo, &canPdu);

    if (CF_Received == true) {
        canPdu.sduDataPtr = cf_payload;
        CanIf_RxIndication(&mailboxInfo, &canPdu);
        cf_payload[0u]++;
        CanIf_RxIndication(&mailboxInfo, &canPdu);
    }
}

static void simple_tx_frame(void) {
    uint8_t payload[8] = {
        1,2,3,4,5,6,7,8
    };

    uint32_t canId = 0x18FF0001UL;
    uint16_t length = 8u;
    Can_Write(canId, payload, length);
}

static void tp_tx_frame(void) {
    uint8_t fc_payload[8] = {
        0x30, 0,0,0,0,0,0,0
    };
    const CanIf_HwType_t mailboxInfo = {
        .canId = 0x18FF0102UL,
        .hoh = 0,
        .controllerId = 0,
    };

    CanPdu_t canPdu = {
        .sduLength = 8,
        .sduDataPtr = fc_payload,
    };

    uint8_t payload[128] = {
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
        1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
    };
    uint32_t canId = 0x18FF0002UL;
    uint16_t length = 128u;
    Can_Write(canId, payload, length);
    Can_MainFunction();

    //Receive FC
    CanIf_RxIndication(&mailboxInfo, &canPdu);
    Can_MainFunction();
    Can_MainFunction();
}


int main(int argc, char *argv[]) {
    Can_Init();

    // simple_rx_frame();
    // tp_rx_frame();
    simple_tx_frame();
    // tp_tx_frame();

    return 0;
}
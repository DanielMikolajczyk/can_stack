#include "Std_Types.h"
#include "CanDrv.h"
#include "CanIf.h"

//TODO change placement of function and proper defines
#define REG_CAN_MB0_RX_ID (0)
#define REG_CAN_MB0_RX_DLC (0)
#define REG_CAN_MB0_RX_DATA_START (0)


void CAN_Mailbox0_Interrupt_Handler(void) {
    uint32_t can_id;
    uint32_t can_dlc;
    uint8_t payload_buffer[64]; // Allocate actual memory (64 bytes for CAN-FD, 8 for Standard)
    CanIf_HwType_t mailboxInfo;
    CanPduInfoType_t canPduInfo;

    // 1. Read Metadata from Registers
    CanDriver_ReadReg(REG_CAN_MB0_RX_ID, &can_id);
    CanDriver_ReadReg(REG_CAN_MB0_RX_DLC, &can_dlc);

    //TODO: DMA
    // 2. Read Data from Registers safely into our memory buffer
    // (Note: Hardware usually has multiple 32-bit data registers,
    // so this function must copy the bytes into your payload_buffer array)
    CanDriver_ReadPayloadRegs(REG_CAN_MB0_RX_DATA_START, payload_buffer, can_dlc);

    // 3. Pack into the Abstract Architectural Structures
    mailboxInfo.canId = can_id;
    mailboxInfo.hoh = 0;           // We know this is Mailbox 0 based on the interrupt
    mailboxInfo.controllerId = 0;  // Controller 0

    canPduInfo.sduLength = can_dlc;   // Assuming your driver already converted DLC code to byte length
    canPduInfo.sduDataPtr = payload_buffer; // Point to our local array!

    // 4. Pass across the boundary!
    // The driver passes the Mailbox info. CanIf will figure out the PDU ID.
    CanIf_RxIndication(&mailboxInfo, &canPduInfo);
}
#include "Can.h"
#include <stdio.h>

// Application callback when a full CAN message or reassembled TP message arrives
void App_OnCanMessageReceived(uint32_t messageId, const uint8_t* payload, uint16_t length) {
    printf("App received MSG ID: 0x%X | Length: %d\n", messageId, length);
}

// Application callback when network state changes
void App_OnCanStateChanged(Can_State_t newState) {
    printf("App Network State Changed: %d\n", newState);
}

int main(int argc, char *argv[]) {
    Can_Init();
    return 0;
}
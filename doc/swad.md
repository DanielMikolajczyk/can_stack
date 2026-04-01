# Software Architecture Document (SWAD) - AUTOSAR-like CAN Stack

## 1. Overview

This document outlines the software architecture for a custom, layered CAN (Controller Area Network) stack. The design is heavily inspired by the AUTOSAR (Automotive Open System Architecture) standard, promoting modularity, scalability, and hardware abstraction.

The stack is designed to handle standard CAN 2.0B and CAN-FD frames, as well as multi-frame communication using an ISO 15765-2 (ISO-TP) like transport protocol.

## 2. Architectural Layers

The stack is organized into three distinct layers, ensuring a clear separation of concerns.



### Layer 1: Application Facade
*   **Purpose:** Provides a single, simplified entry point for the main application. It hides the complexity of the underlying communication services.
*   **Module:** `Can.c`

### Layer 2: Communication Services
*   **Purpose:** Manages network state, data flow, and complex protocols. This layer is responsible for the "logic" of the communication.
*   **Modules:**
    *   `CanSM.c` (CAN State Manager)
    *   `CanTp.c` (CAN Transport Protocol)

### Layer 3: Hardware Abstraction
*   **Purpose:** Decouples the upper layers from the specific microcontroller hardware. This layer is responsible for interacting with physical registers and peripherals.
*   **Modules:**
    *   `CanIf.c` (CAN Interface)
    *   `CanDrv.c` (CAN Driver - Mock/MCAL)
    *   `CanTrcv.c` (CAN Transceiver)

### Configuration
*   **Purpose:** Provides static, link-time configuration for all modules, defining messages, timings, and routing rules.
*   **Modules:** `CanCfg.c`, `CanCfg.h`

---

## 3. Module Descriptions

### `Can.c` (Facade)
*   **Responsibility:** The sole interface for the application layer. It orchestrates calls to the lower-level modules.
*   **Key Functions:**
    *   `Can_Init()`: Initializes all modules in the stack.
    *   `Can_Write()`: The primary data transmission function. It intelligently routes a message to `CanIf` (for single frames) or `CanTp` (for multi-frame messages) based on the static configuration.
    *   `Can_RequestState()`: Allows the application to request network state changes (e.g., go ONLINE or SLEEP).
    *   `Can_MainFunction()`: The main periodic task for the entire stack. It calls the main functions of `CanSM` and `CanTp`.
*   **Interactions:**
    *   **Calls:** `CanSM`, `CanTp`, `CanIf`.
    *   **Is Called By:** The main application (`main.c`).
    *   **Data Flow:** Forwards application data downwards and provides `App_OnCanMessageReceived` callbacks for data flowing upwards.

### `CanSM.c` (CAN State Manager)
*   **Responsibility:** Manages the state of the CAN network (UNINIT, OFFLINE, ONLINE, SLEEP, BUS_OFF). It handles state transition requests and reacts to network events.
*   **Key Functions:**
    *   `CanSM_MainFunction()`: Processes state transitions and handles Bus-Off recovery timers.
    *   `CanSM_RequestState()`: Receives state change requests from the `Can` module.
    *   `CanSM_ReportBusOff()`: An event callback triggered by the lower layers when a Bus-Off error is detected.
    *   `CanSM_StartTimer()` / `CanSM_GetTimer()`: Manages internal software timers for recovery sequences.
*   **Interactions:**
    *   **Calls:** `CanIf_SetControllerMode()` to enable/disable the hardware.
    *   **Is Called By:** `Can_MainFunction`.

### `CanTp.c` (CAN Transport Protocol)
*   **Responsibility:** Implements ISO 15765-2 for segmenting large messages for transmission and reassembling them upon reception.
*   **Key Functions:**
    *   `CanTp_Transmit()`: Receives a large data payload and queues it for segmentation.
    *   `CanTp_RxIndication()`: Parses incoming CAN frames to identify them as First Frames (FF), Consecutive Frames (CF), or Flow Control (FC) frames.
    *   `CanTp_MainFunction()`: The periodic task that drives the state machines for both sending and receiving segmented messages.
*   **Interactions:**
    *   **Calls:** `CanIf_Transmit()` to send its individual frames (FF, CF, FC). Calls `Can_RxIndication()` when a full message is reassembled.
    *   **Is Called By:** `Can_Write()` (for TX) and `CanIf_RxIndication()` (for RX).

### `CanIf.c` (CAN Interface)
*   **Responsibility:** The central routing hub of the stack. It abstracts the specific CAN controller hardware from the service layer. It multiplexes transmit requests and demultiplexes receive indications.
*   **Key Functions:**
    *   `CanIf_Transmit()`: Takes a PDU from `Can` or `CanTp` and passes it to the `CanDrv` for physical transmission.
    *   `CanIf_RxIndication()`: Called by `CanDrv` from an interrupt context. It finds the corresponding PDU configuration and routes the frame up to `Can` or `CanTp`.
    *   `CanIf_SetControllerMode()`: Relays state change commands (START, STOP, SLEEP) from `CanSM` to the `CanDrv`.
*   **Interactions:**
    *   **Calls:** `CanDrv_Transmit()`, `CanDrv_SetControllerMode()`, `CanTp_RxIndication()`, `Can_RxIndication()`.
    *   **Is Called By:** `Can`, `CanTp`, `CanSM`, and `CanDrv`.

### `CanDrv.c` (CAN Driver)
*   **Responsibility:** The lowest software layer, representing the MCAL (Microcontroller Abstraction Layer). It is responsible for directly manipulating hardware registers of the CAN peripheral.
*   **Key Functions:**
    *   `CanDriver_Transmit()`: Writes a CAN frame's ID, DLC, and payload into the hardware's transmit mailboxes/FIFOs.
    *   `CanDriver_SetControllerMode()`: Sets hardware bits to start, stop, or sleep the CAN controller.
    *   `CAN_Mailbox0_Interrupt_Handler()`: The Interrupt Service Routine (ISR) that is triggered by the hardware upon a successful message reception.
*   **Interactions:**
    *   **Calls:** `CanIf_RxIndication()` to pass received data up the stack.
    *   **Is Called By:** `CanIf`.

### `CanCfg.c` / `CanCfg.h` (Configuration)
*   **Responsibility:** Provides a single source of truth for the entire CAN stack's configuration. All message properties (ID, length, protocol, periodicity) and hardware object mappings are defined here.
*   **Key Structures:**
    *   `Can_TxPduConfigType`: Defines a message to be transmitted.
    *   `Can_RxPduConfigType`: Defines a message to be received.
    *   `Can_ConfigType`: The root configuration structure that holds pointers to all other configuration tables.
*   **Interactions:** This module contains only constant data. It is read by almost all other modules (`Can`, `CanIf`, `CanTp`) to make runtime decisions.

---

## 4. Key Communication Scenarios

### A. Standard Frame Transmission
1.  `App` -> `Can_Write(ID, data, len)`
2.  `Can.c`: Finds the PDU config in `CanCfg`. Determines it's a `CAN_IF` message.
3.  `Can.c` -> `CanIf_Transmit(PduInfo)`
4.  `CanIf.c` -> `CanDrv_Transmit(HwInfo, PduInfo)`
5.  `CanDrv.c`: Writes data to hardware registers.

### B. Multi-Frame (TP) Transmission
1.  `App` -> `Can_Write(ID, data, len)`
2.  `Can.c`: Finds the PDU config. Determines it's a `CAN_TP` message.
3.  `Can.c` -> `CanTp_Transmit(PduInfo)`
4.  `CanTp.c`: Allocates a TX channel and sets its state to `SEND_FF`.
5.  `CanTp_MainFunction()`:
    *   Sees `SEND_FF` state, builds and sends the First Frame via `CanIf_Transmit()`.
    *   Transitions state to `WAIT_FC`.
6.  (Later) `CanIf_RxIndication()` receives a Flow Control frame, calls `CanTp_RxIndication()`.
7.  `CanTp_RxIndication()`: Parses the FC frame, updates the TX channel, and sets its state to `SEND_CF`.
8.  `CanTp_MainFunction()`: Sees `SEND_CF` state, sends Consecutive Frames via `CanIf_Transmit()` until the message is complete.

### C. Frame Reception
1.  **Hardware:** A message is received. `CAN_Mailbox0_Interrupt_Handler()` is triggered.
2.  `CanDrv.c` (ISR): Reads ID and payload from hardware registers.
3.  `CanDrv.c` -> `CanIf_RxIndication(HwInfo, PduInfo)`
4.  `CanIf.c`: Finds the PDU config in `CanCfg` based on the CAN ID.
5.  `CanIf.c`: Checks the `protocol` field.
    *   If `CAN_IF`: `CanIf.c` -> `Can_RxIndication()` -> `App_OnCanMessageReceived()`.
    *   If `CAN_TP`: `CanIf.c` -> `CanTp_RxIndication()`. `CanTp` module then handles the reassembly process.

### D. Bus-Off Event
1.  **Hardware:** Detects a Bus-Off state and triggers an error interrupt.
2.  `CanDrv.c` (ISR): Calls `CanSM_ReportBusOff()`.
3.  `CanSM.c`: Sets `CanSM_BusOffPending` flag to `true`.
4.  `CanSM_MainFunction()`:
    *   Detects the pending flag.
    *   Transitions internal state to `CANSM_STATE_BUS_OFF`.
    *   Calls `CanIf_SetControllerMode(STOPPED)`.
    *   Starts the `CANSM_TIMER_BUS_OFF` for recovery.
5.  (Later) `CanSM_MainFunction()`:
    *   Sees the recovery timer has expired (`CanSM_GetTimer()` is true).
    *   Calls `CanIf_SetControllerMode(STARTED)` to attempt to rejoin the bus.
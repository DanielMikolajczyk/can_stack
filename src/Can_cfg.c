/**
 * @file    can_cfg.c
 * @brief   CAN Driver Configuration - Tx and Rx Message Definitions
 *
 * @details AUTOSAR-like CAN configuration for a mixed CAN 2.0B (29-bit ID)
 *          and CAN FD stack. Covers Engine/Powertrain and Vehicle Speed/Wheel
 *          data messages.
 *
 *          Naming conventions follow AUTOSAR Com/CanIf patterns:
 *            - CanCfg_TxPdu  : Transmit PDU descriptors
 *            - CanCfg_RxPdu  : Receive  PDU descriptors
 *            - CanMessageObjects : Hardware message-object table
 */

#include "Can_cfg.h"

/* =========================================================================
 * 1.  SYMBOLIC IDENTIFIERS
 * ========================================================================= */

/* --- Tx PDU IDs (indices into CanCfg_TxPdu[]) ---------------------------- */
#define CAN_TX_PDU_ENGINE_STATUS        0U  /* Engine RPM, torque, throttle  */
#define CAN_TX_PDU_FUEL_INJECTION       1U  /* Injection timing & fuel rate  */
#define CAN_TX_PDU_VEHICLE_SPEED        2U  /* Vehicle reference speed       */
#define CAN_TX_PDU_WHEEL_SPEED          3U  /* Four-wheel speed signals (FD) */

/* --- Rx PDU IDs (indices into CanCfg_RxPdu[]) ---------------------------- */
#define CAN_RX_PDU_ENGINE_CTRL          0U  /* Torque request from TCU/BCM   */
#define CAN_RX_PDU_BRAKE_PRESSURE       1U  /* Brake pressure from ABS ECU   */
#define CAN_RX_PDU_WHEEL_TORQUE         2U  /* Per-wheel torque feedback (FD)*/
#define CAN_RX_PDU_THROTTLE_POS         3U  /* Accelerator pedal position    */

/* =========================================================================
 * 2.  TRANSMIT PDU CONFIGURATION TABLE
 *
 *  Fields:
 *    .PduId          – Internal software PDU handle
 *    .CanId          – CAN frame identifier (29-bit extended, or FD id)
 *    .CanIdType      – CAN_ID_TYPE_EXTENDED (2.0B) | CAN_ID_TYPE_FD
 *    .FrameType      – CAN_FRAME_CLASSIC | CAN_FRAME_FD
 *    .Length            – Data length code (bytes; FD allows up to 64)
 *    .TxConfirmation – Callback invoked after successful transmission
 *    .CyclicPeriodMs – 0 = event-driven; >0 = periodic transmission (ms)
 *    .HwObjectRef    – Index into CanMessageObjects[] for the Tx buffer
 * ========================================================================= */
const Can_TxPduConfigType CanCfg_TxPdu[CAN_NUM_TX_PDUS] =
{
    /* ------------------------------------------------------------------ */
    /* PDU 0 – ENGINE_STATUS  (CAN 2.0B, 29-bit, 8 bytes, 10 ms cyclic)  */
    /*   Signals:                                                          */
    /*     [0..1]  EngineRPM        uint16, 0–16383 rpm, factor 0.25      */
    /*     [2..3]  EngineTorque     int16,  –3276–3276 Nm, factor 0.1     */
    /*     [4]     ThrottlePosition uint8,  0–100 %                       */
    /*     [5]     EngineTemp       int8,   –40–215 °C, offset –40        */
    /*     [6]     EngineStatus     uint8,  bitmask (run/crank/fault …)   */
    /*     [7]     Checksum         uint8,  XOR of bytes 0–6              */
    /* ------------------------------------------------------------------ */
    {
        .canId        = 0x18FF0001UL,
        .canIdType    = CAN_ID_TYPE_EXTENDED,
        .frameType    = CAN_FRAME_CLASSIC,
        .protocol     = CAN_IF,
        .length       = 8u,
        .CyclicPeriodMs = 10U,
        .globalTxId      = 0u,
        .HwObjectRef    = CAN_HW_OBJ_TX_0,
        .TxConfirmation = NULL_PTR,
    },


    /* ------------------------------------------------------------------ */
    /* PDU 1 – FUEL_INJECTION  (CAN 2.0B, 29-bit, 8 bytes, 5 ms cyclic) */
    /*   Signals:                                                          */
    /*     [0..1]  InjectionTiming  uint16, 0–65535 µs                    */
    /*     [2..3]  FuelFlowRate     uint16, 0–655.35 mg/stroke, ×0.01    */
    /*     [4]     InjectorCylMask  uint8,  bitmask cyl 1–8 active        */
    /*     [5]     RailPressure     uint8,  0–255 MPa, factor 1           */
    /*     [6..7]  Reserved         uint16, 0x0000                        */
    /* ------------------------------------------------------------------ */
    {
        .canId        = 0x18FF0002UL,
        .canIdType    = CAN_ID_TYPE_EXTENDED,
        .frameType    = CAN_FRAME_CLASSIC,
        .protocol     = CAN_IF,
        .length       = 8u,
        .CyclicPeriodMs = 20U,
        .globalTxId      = 1u,
        .HwObjectRef    = CAN_HW_OBJ_TX_0,
        .TxConfirmation = NULL_PTR,
    },

    /* ------------------------------------------------------------------ */
    /* PDU 2 – VEHICLE_SPEED  (CAN 2.0B, 29-bit, 8 bytes, 20 ms cyclic) */
    /*   Signals:                                                          */
    /*     [0..1]  VehicleSpeed     uint16, 0–655.35 km/h, factor 0.01   */
    /*     [2..3]  LongAcceleration int16,  –327.68–327.67 m/s², ×0.01  */
    /*     [4..5]  YawRate          int16,  –327.68–327.67 °/s, ×0.01   */
    /*     [6]     GearPosition     uint8,  0=N,1–8=gear, 9=R, 10=P      */
    /*     [7]     Checksum         uint8,  XOR of bytes 0–6             */
    /* ------------------------------------------------------------------ */
    {
        .canId        = 0x18FF0010UL,
        .canIdType    = CAN_ID_TYPE_EXTENDED,
        .frameType    = CAN_FRAME_CLASSIC,
        .protocol     = CAN_IF,
        .length       = 8u,
        .CyclicPeriodMs = 0U,
        .globalTxId      = 2u,
        .HwObjectRef    = CAN_HW_OBJ_TX_0,
        .TxConfirmation = NULL_PTR,
    },

    /* ------------------------------------------------------------------ */
    /* PDU 3 – WHEEL_SPEED  (CAN FD, 29-bit, 16 bytes, 10 ms cyclic)    */
    /*   Uses CAN FD for higher payload without extra frames.              */
    /*   Signals (all uint16, factor 0.01 rpm):                           */
    /*     [0..1]  WheelSpeedFL                                           */
    /*     [2..3]  WheelSpeedFR                                           */
    /*     [4..5]  WheelSpeedRL                                           */
    /*     [6..7]  WheelSpeedRR                                           */
    /*     [8..9]  WheelAccelFL    int16, ×0.01 rpm/s                    */
    /*     [10..11] WheelAccelFR                                          */
    /*     [12..13] WheelAccelRL                                          */
    /*     [14..15] WheelAccelRR                                          */
    /* ------------------------------------------------------------------ */
    {
        .canId        = 0x18FF0011UL,
        .canIdType    = CAN_ID_TYPE_FD,
        .frameType    = CAN_FRAME_FD,
        .protocol     = CAN_IF,
        .length       = 8u,
        .CyclicPeriodMs = 0U,
        .globalTxId      = 3u,
        .HwObjectRef    = CAN_HW_OBJ_TX_0,
        .TxConfirmation = NULL_PTR,
    },

    {
        .canId        = 0x18FF0102UL,
        .canIdType    = CAN_ID_TYPE_EXTENDED,
        .frameType    = CAN_FRAME_CLASSIC,
        .protocol     = CAN_IF,
        .length       = 8u,
        .CyclicPeriodMs = 0U,
        .globalTxId      = 3u,
        .HwObjectRef    = CAN_HW_OBJ_TX_0,
        .TxConfirmation = NULL_PTR,
    },
};

/* =========================================================================
 * 3.  RECEIVE PDU CONFIGURATION TABLE
 *
 *  Fields:
 *    .PduId          – Internal software PDU handle
 *    .CanId          – Expected CAN frame identifier
 *    .CanIdMask      – Acceptance mask (0xFFFFFFFF = exact match)
 *    .CanIdType      – CAN_ID_TYPE_EXTENDED | CAN_ID_TYPE_FD
 *    .FrameType      – CAN_FRAME_CLASSIC | CAN_FRAME_FD
 *    .Length            – Expected data length (hard-checked on reception)
 *    .RxIndication   – Callback invoked with received data pointer
 *    .TimeoutMs      – 0 = no supervision; >0 = deadline monitoring (ms)
 *    .HwObjectRef    – Index into CanMessageObjects[] for the Rx FIFO/buf
 * ========================================================================= */
const Can_RxPduConfigType CanCfg_RxPdu[CAN_NUM_RX_PDUS] =
{
    /* ------------------------------------------------------------------ */
    /* PDU 0 – ENGINE_CTRL  (CAN 2.0B, 29-bit, 8 bytes)                  */
    /*   Source: TCU / BCM torque coordinator                             */
    /*   Signals:                                                          */
    /*     [0..1]  RequestedTorque  int16, –3276–3276 Nm, factor 0.1     */
    /*     [2]     EngineMode       uint8, 0=normal,1=sport,2=eco,3=limp  */
    /*     [3]     CruiseSetSpeed   uint8, 0–250 km/h                     */
    /*     [4]     CruiseControl    uint8, bitmask (en/resume/cancel …)   */
    /*     [5..6]  Reserved                                               */
    /*     [7]     Checksum         uint8, XOR of bytes 0–6              */
    /* ------------------------------------------------------------------ */
    {
        .canId        = 0x18FF0100UL,
        .canIdType    = CAN_ID_TYPE_EXTENDED,
        .frameType    = CAN_FRAME_CLASSIC,
        .protocol     = CAN_IF,
        .length       = 8u,
        .timeoutMs    = 50U,
        .globalRxId   = 0u
    },

    /* ------------------------------------------------------------------ */
    /* PDU 1 – BRAKE_PRESSURE  (CAN 2.0B, 29-bit, 8 bytes)              */
    /*   Source: ABS / ESC ECU                                            */
    /*   Signals:                                                          */
    /*     [0..1]  BrakePressureFront  uint16, 0–250 bar, factor 0.01    */
    /*     [2..3]  BrakePressureRear   uint16, 0–250 bar, factor 0.01    */
    /*     [4]     ABSActive           uint8,  bitmask per wheel          */
    /*     [5]     ESCActive           uint8,  bitmask per wheel          */
    /*     [6]     BrakeTemp           uint8,  0–255 °C                  */
    /*     [7]     Checksum            uint8,  XOR of bytes 0–6          */
    /* ------------------------------------------------------------------ */
    {
        .canId        = 0x18FF0101UL,
        .canIdType    = CAN_ID_TYPE_EXTENDED,
        .frameType    = CAN_FRAME_CLASSIC,
        .protocol     = CAN_IF,
        .length       = 8u,
        .timeoutMs    = 20u,
        .globalRxId   = 0u
    },

    /* ------------------------------------------------------------------ */
    /* PDU 2 – WHEEL_TORQUE  (CAN FD, 29-bit, 16 bytes)                 */
    /*   Source: Torque vectoring / motor controller                      */
    /*   Signals (all int16, factor 0.1 Nm):                             */
    /*     [0..1]  WheelTorqueFL                                          */
    /*     [2..3]  WheelTorqueFR                                          */
    /*     [4..5]  WheelTorqueRL                                          */
    /*     [6..7]  WheelTorqueRR                                          */
    /*     [8..9]  WheelSlipFL     int16, ×0.01 %                        */
    /*     [10..11] WheelSlipFR                                           */
    /*     [12..13] WheelSlipRL                                           */
    /*     [14..15] WheelSlipRR                                           */
    /* ------------------------------------------------------------------ */
    {
        .canId        = 0x18FF0110UL,
        .canIdType    = CAN_ID_TYPE_FD,
        .frameType    = CAN_FRAME_FD,
        .protocol     = CAN_IF,
        .length       = 16u,
        .timeoutMs    = 20u,
        .globalRxId   = 0u
    },

    /* ------------------------------------------------------------------ */
    /* PDU 3 – THROTTLE_POS  (CAN 2.0B, 29-bit, 8 bytes)                */
    /*   Source: Pedal / APPS sensor ECU                                  */
    /*   Signals:                                                          */
    /*     [0]     AccelPedalPos1   uint8,  0–100 %, sensor 1            */
    /*     [1]     AccelPedalPos2   uint8,  0–100 %, sensor 2 (redundant)*/
    /*     [2]     KickdownSwitch   uint8,  0=off, 1=on                  */
    /*     [3]     PedalFaultMask   uint8,  bitmask                      */
    /*     [4..5]  Reserved                                               */
    /*     [6]     Counter          uint8,  rolling 0–255                */
    /*     [7]     Checksum         uint8,  XOR of bytes 0–6             */
    /* ------------------------------------------------------------------ */
    {
        .canId        = 0x18FF0102UL,
        .canIdType    = CAN_ID_TYPE_FD,
        .frameType    = CAN_FRAME_FD,
        .protocol     = CAN_TP,
        .length       = 184u,
        .timeoutMs    = 10u,
        .globalRxId   = 0u
    },
};

/* =========================================================================
 * 4.  HARDWARE MESSAGE OBJECT TABLE
 *
 *  Maps logical Tx/Rx buffers to physical controller message objects.
 *  Adjust HwObjId to match your MCU's CAN controller register layout.
 * ========================================================================= */
const Can_HwObjectConfigType CanMessageObjects[CAN_NUM_HW_OBJECTS] =
{
    /* Tx dedicated buffers */
    { .HwObjId = 0U,  .Direction = CAN_DIRECTION_TX, .FrameType = CAN_FRAME_CLASSIC, .FifoDepth = 1U },
    { .HwObjId = 1U,  .Direction = CAN_DIRECTION_TX, .FrameType = CAN_FRAME_CLASSIC, .FifoDepth = 1U },
    { .HwObjId = 2U,  .Direction = CAN_DIRECTION_TX, .FrameType = CAN_FRAME_CLASSIC, .FifoDepth = 1U },
    { .HwObjId = 3U,  .Direction = CAN_DIRECTION_TX, .FrameType = CAN_FRAME_FD,      .FifoDepth = 1U },

    /* Rx FIFOs */
    { .HwObjId = 16U, .Direction = CAN_DIRECTION_RX, .FrameType = CAN_FRAME_CLASSIC, .FifoDepth = 8U },  /* FIFO 0 */
    { .HwObjId = 17U, .Direction = CAN_DIRECTION_RX, .FrameType = CAN_FRAME_FD,      .FifoDepth = 8U }   /* FIFO 1 */
};

/* =========================================================================
 * 5.  MODULE-LEVEL CONFIGURATION ROOT
 * ========================================================================= */
const Can_ConfigType CanConfig =
{
    .TxPduConfig        = CanCfg_TxPdu,
    .NumTxPdus          = CAN_NUM_TX_PDUS,
    .RxPduConfig        = CanCfg_RxPdu,
    .NumRxPdus          = CAN_NUM_RX_PDUS,
    .HwObjectConfig     = CanMessageObjects,
    .NumHwObjects       = CAN_NUM_HW_OBJECTS,
    .BaudrateClassic    = 500000U,    /* 500 kbit/s for CAN 2.0B frames     */
    .BaudrateFd         = 2000000U,   /* 2 Mbit/s data phase for CAN FD     */
    .ClockFreqHz        = 80000000UL  /* 80 MHz peripheral clock – adjust!  */
};

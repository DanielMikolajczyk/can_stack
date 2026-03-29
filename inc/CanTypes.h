#ifndef CAN_TYPES_H
#define CAN_TYPES_H

#include "Std_Types.h"

/**************************** DEFINES *****************************/

#define CAN_FD_MAX_SINGLE_FRAME_SIZE (64U)
#define CAN_MAX_SINGLE_FRAME_SIZE    (8U)

/**************************** ENUMS *****************************/

typedef enum
{
    CAN_ID_TYPE_STANDARD = 0U,   /* 11-bit (2.0A) */
    CAN_ID_TYPE_EXTENDED = 1U,   /* 29-bit (2.0B) */
    CAN_ID_TYPE_FD       = 2U    /* 29-bit + BRS  */
} Can_IdType;

/** CAN frame format */
typedef enum
{
    CAN_FRAME_CLASSIC = 0U,      /* Classic CAN 2.0B, max 8 bytes  */
    CAN_FRAME_FD      = 1U       /* CAN FD, up to 64 bytes         */
} Can_FrameType;

/** Message object transfer direction */
typedef enum
{
    CAN_DIRECTION_TX = 0U,
    CAN_DIRECTION_RX = 1U
} Can_DirectionType;

/** Message object transfer direction */
typedef enum
{
    CAN_IF = 0U,
    CAN_TP = 1U
} Can_ProtocolType;

typedef enum {
    CAN_STATE_OFFLINE,
    CAN_STATE_ONLINE,
    CAN_STATE_SLEEP,
    CAN_STATE_ERROR
} Can_State_t;

typedef enum {
    CANSM_STATE_UNINIT,
    CANSM_STATE_OFFLINE,
    CANSM_STATE_ONLINE,
    CANSM_STATE_SLEEP,
    CANSM_STATE_BUS_OFF
} CanSM_NetworkState_t;

/**************************** TYPEDEFS *****************************/

/** Called by the driver after a Tx frame has been successfully transmitted */
typedef void (*Can_TxConfirmationType)(uint8_t PduId);

/** Called by the driver when a matching Rx frame has been received */
typedef void (*Can_RxIndicationType)(uint8_t PduId, const uint8_t *Data, uint8_t Dlc);


typedef struct {
    uint32_t canId;
    uint32_t hoh;
    uint32_t controllerId;
} CanIf_HwType_t;

typedef struct {
    uint32_t sduLength;
    uint8_t* sduDataPtr;
} CanPdu_t;

typedef struct
{
    uint32_t              canId;
    Can_IdType            canIdType;
    Can_FrameType         frameType;
    Can_ProtocolType      protocol;
    uint32_t              length;
    uint16_t              CyclicPeriodMs;   /* 0 = event-driven */
    uint32_t              globalTxId;       /* 0 = no supervision */
    uint8_t               HwObjectRef;
    Can_TxConfirmationType TxConfirmation;
} Can_TxPduConfigType;

typedef struct
{
    uint32_t              canId;
    Can_IdType            canIdType;
    Can_FrameType         frameType;
    Can_ProtocolType      protocol;
    uint32_t              length;
    uint16_t              timeoutMs;        /* 0 = no supervision */
    uint32_t              globalRxId;       /* 0 = no supervision */
} Can_RxPduConfigType;

typedef struct
{
    uint8_t             HwObjId;
    Can_DirectionType   Direction;
    Can_FrameType       FrameType;
    uint8_t             FifoDepth;
} Can_HwObjectConfigType;

typedef struct
{
    const Can_TxPduConfigType   *TxPduConfig;
    uint8_t                      NumTxPdus;
    const Can_RxPduConfigType   *RxPduConfig;
    uint8_t                      NumRxPdus;
    const Can_HwObjectConfigType *HwObjectConfig;
    uint8_t                      NumHwObjects;
    uint32_t                     BaudrateClassic;  /* bps */
    uint32_t                     BaudrateFd;       /* bps, data phase */
    uint32_t                     ClockFreqHz;
} Can_ConfigType;


#endif /*CAN_TYPES_H*/
/**
 * @file    CanCfg.h
 * @brief   CAN Driver Configuration – Type definitions, constants, and
 *          callback prototypes for the custom AUTOSAR-like CAN stack.
 */

#ifndef CAN_CFG_H
#define CAN_CFG_H

#include "Std_Types.h"
#include "CanTypes.h"

/* =========================================================================
 * 1.  CONFIGURATION SIZES
 * ========================================================================= */
#define CAN_NUM_TX_PDUS     5U
#define CAN_NUM_RX_PDUS     4U
#define CAN_NUM_HW_OBJECTS  6U   /* 4 Tx buffers + 2 Rx FIFOs */

/* =========================================================================
 * 2.  ENUM TYPES
 * ========================================================================= */

/** CAN identifier type */


/* =========================================================================
 * 3.  HARDWARE OBJECT SYMBOLIC NAMES
 * ========================================================================= */
#define CAN_HW_OBJ_TX_0    0U
#define CAN_HW_OBJ_TX_1    1U
#define CAN_HW_OBJ_TX_2    2U
#define CAN_HW_OBJ_TX_3    3U
#define CAN_HW_OBJ_RX_FIFO0 4U
#define CAN_HW_OBJ_RX_FIFO1 5U

/* =========================================================================
 * 4.  CALLBACK TYPEDEFS
 * ========================================================================= */

/* =========================================================================
 * 5.  PDU CONFIGURATION TYPES
 * ========================================================================= */



/* =========================================================================
 * 7.  MODULE ROOT CONFIGURATION TYPE
 * ========================================================================= */


/* =========================================================================
 * 8.  EXTERN DECLARATIONS
 * ========================================================================= */

extern const Can_TxPduConfigType     CanCfg_TxPdu[CAN_NUM_TX_PDUS];
extern const Can_RxPduConfigType     CanCfg_RxPdu[CAN_NUM_RX_PDUS];
extern const Can_HwObjectConfigType  CanMessageObjects[CAN_NUM_HW_OBJECTS];
extern const Can_ConfigType          CanConfig;

/* =========================================================================
 * 9.  TX CONFIRMATION CALLBACK PROTOTYPES
 * ========================================================================= */

/* =========================================================================
 * 10. RX INDICATION CALLBACK PROTOTYPES
 * ========================================================================= */

#endif /* CAN_CFG_H */

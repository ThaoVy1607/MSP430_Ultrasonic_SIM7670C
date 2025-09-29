/* --COPYRIGHT--,BSD
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#ifndef __TI_SERIALCOMM_H__
#define __TI_SERIALCOMM_H__

//
// Include files
//
#include <stdint.h>
#include <stdbool.h>
#include "hal.h"

//
// Macros and definitions
//
#define SERIALCOMM_HEADER               (0xA5)
#define SERIALCOMM_WAKE_FROM_PC         (0x55)
typedef enum t_SC_CMD_TO_PC
{
    CMD_TO_PC_TOF =0,       // 0
    CMD_TO_PC_STD_DATA,     // 1
    CMD_TO_PC_ABS_TOF,      // 2
    CMD_TO_PC_VOL,          // 3
    CMD_TO_PC_UPS_DNS,      // 4
    CMD_TO_PC_REQ_CONFIG,   // 5
    CMD_TO_PC_PARAM_OK,     // 6
    CMD_TO_PC_REQ_USER_SEL, // 7
    CMD_TO_PC_USER_SEL_OK,  // 8
    CMD_TO_PC_TO_PC_END     // 9
}t_SC_CMD_TO_PC;

typedef enum t_SC_CMD_FROM_PC
{
    CMD_FROM_PC_USER_SEL =0x80,
    CMD_FROM_PC_ARRAY_DATA =0x85,    
    CMD_FROM_PC_PARAM_CONFIG =0x81,
    CMD_FROM_PC_END =0x88
}t_SC_CMD_FROM_PC;

typedef enum {
    SERIALCOMM_NO_ERROR = 0,
    SERIALCOMM_ERR_INVALID_CMD, 
    SERIALCOMM_ERR_INVALID_PACKETLEN, 
}SC_Error_t;  
   
//
// Type definitions
//


/*! State machine of Serial Communication */
typedef enum
{
    SerialComm_Idle=0,              /*! SerialComm is idle          */
    SerialComm_WaitHdr,             /*! SerialComm is waiting for header */
    SerialComm_WaitCMD,             /*! SerialComm is waiting for Command */
    SerialComm_WaitLen1,            /*! SerialComm is waiting for 1st byte Len */
    SerialComm_WaitLen2,            /*! SerialComm is waiting for 2nd byte Len*/
    SerialComm_RxData,              /*! SerialComm is receiving data  */
    SerialComm_Error,               /*! SerialComm error */
    SerialComm_CMDReceived,         /*! Command was received */
}t_SerialComm_State;

typedef struct t_SC_Payload_list_item t_SC_Payload_list_item;
struct t_SC_Payload_list_item
{
    uint8_t                         *PayloadPtr;
    uint16_t                        Len;
    t_SC_Payload_list_item          *Next_item;
};

typedef struct
{
    uint8_t                 Cmd;
    t_SC_Payload_list_item  *PayloadFirst;
}t_SerialComm_Packet;


/*! Handler called when an error is detected */
typedef void (*Error_Function_t)(uint8_t Error);

/*! Handler called when a full command is received */
typedef void (*Rx_Complete_Function_t)(t_SerialComm_Packet * RxPacket);

//
// Functions 
// 

extern void TI_SerialComm_Init(void);
extern void TI_SerialComm_RxEnable(void);
extern void TI_SerialComm_RxDisable(void);
extern void TI_SerialComm_SetErrorHandler(Error_Function_t ErrorHandler);
extern void TI_SerialComm_PacketCompleteHandler(Rx_Complete_Function_t RxComHandler);
extern int8_t TI_SerialComm_StartPacketReception(t_SerialComm_Packet *RXPacket, uint16_t MaxLen);
extern t_SerialComm_State TI_SerialComm_GetState(void);
extern uint8_t TI_SerialComm_GetRxAvail(void);
extern bool TI_SerialComm_PacketComplete(void);
extern bool TI_SerialComm_WaitForRXPacket_wTimeout(uint32_t timeout);
extern void TI_SerialComm_TxByte(uint8_t Data);
extern void TI_SerialComm_TxString(uint8_t *Data);
extern void TI_SerialComm_TxHexByte(uint8_t Data);
extern void TI_SerialComm_TxArray(uint8_t *Data, uint16_t len);
extern void TI_SerialComm_TxHexArray(uint8_t *Data, uint16_t len);
extern void TI_SerialComm_TxPacket(t_SerialComm_Packet *TxPacket);
extern bool TI_SerialComm_RxPeekByte(uint8_t *byte);

#endif //__TI_SERIALCOMM_H__

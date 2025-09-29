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

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "TI_SerialComm.h"
#include "msp430.h"

//
// Type definitions
//
/*! Structure for SerialComm control*/
typedef struct
{
    volatile t_SerialComm_State eState; /*! Current State of SerialComm   */
    volatile uint16_t RxIndex;           /*! Current Index of RX data      */
    t_SerialComm_Packet *pRXPacket;     /*! Pointer to RX Packet          */
    uint16_t RX_MaxLen;                  /*! Maximum lenght of RX data     */
}t_SerialComm_struct;

//
//  Global variables
//
static Error_Function_t pError_Handler;     /*! Pointer to Error callback */
static Rx_Complete_Function_t pRxComp_Handler;
static t_SerialComm_struct s_SerialComm;    /*! Serial Comm structure     */


//
// Function definition
//
//*****************************************************************************
//
//! Initialize Serial Communication
//!
//! \param busClk  Clock frequency in Hz
//!
//! Initializes UART at 9600bps
//!
//! \return none
//
// *****************************************************************************
void TI_SerialComm_Init(void)
{
    hal_uart_Init();
    s_SerialComm.eState = SerialComm_Idle;
    pError_Handler = NULL;
    pRxComp_Handler = NULL;

}

void TI_SerialComm_RxEnable(void)
{
    hal_uart_RxEnableIntMode();
}

void TI_SerialComm_RxDisable(void)
{
    hal_uart_RxDisableIntMode();
}


//*****************************************************************************
//
//! Set Error callback
//!
//! \param ErrorHandler  Pointer to callback function
//!
//! The error callback will be called when an exception is found by the
//! serial communication interface
//!
//! \return none
//
// *****************************************************************************
void TI_SerialComm_SetErrorHandler(Error_Function_t ErrorHandler)
{
    pError_Handler =  ErrorHandler;
}

void TI_SerialComm_PacketCompleteHandler(Rx_Complete_Function_t RxComHandler)
{
    pRxComp_Handler = RxComHandler;
}

//*****************************************************************************
//
//! Start packet reception
//!
//! \param RXBuf  Pointer to reception buffer
//! \param RXLen  Maximum lenght of the reception buffer
//!
//! Initializes variables to be ready to receive a new packet
//!
//! \return -1 if error, 0 if OK
//
// *****************************************************************************
int8_t TI_SerialComm_StartPacketReception(t_SerialComm_Packet *RXPacket, uint16_t MaxLen)
{
    s_SerialComm.eState = SerialComm_WaitHdr;
    s_SerialComm.RX_MaxLen = MaxLen;
    s_SerialComm.pRXPacket = RXPacket;
    s_SerialComm.RxIndex = 0;
    if (RXPacket->PayloadFirst == NULL)
    {
        return -1;  // List should have 1 member
    }

    s_SerialComm.pRXPacket->PayloadFirst->Len = 0;

    return 0;
}

//*****************************************************************************
//
//! Get state of serial comm
//!
//! \return eState of Serial Comm (check t_SerialComm_struct)
//
// *****************************************************************************
t_SerialComm_State TI_SerialComm_GetState(void)
{
    return s_SerialComm.eState;
}

//*****************************************************************************
//
//! Checks if a packet has been received completely
//!
//! \return true if a packet was received, false if it hasn't
//
// *****************************************************************************
bool TI_SerialComm_PacketComplete(void)
{
    if (s_SerialComm.eState == SerialComm_CMDReceived)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool TI_SerialComm_WaitForRXPacket_wTimeout(uint32_t timeout)
{
    while (timeout--)
    {
        hal_system_WatchdogFeed();
        if (TI_SerialComm_PacketComplete() == true)
        {
            return true;
        }
    }
    return false;
}

//*****************************************************************************
//
//! Get number of received bytes
//!
//! \return number of bytes received by Serial Comm
//
// *****************************************************************************
uint8_t TI_SerialComm_GetRxAvail(void)
{
    return s_SerialComm.RxIndex;
}

//*****************************************************************************
//
//! Transmit byte
//!
//! \param Data     Byte to be transmitted
//!
//! Transmits a byte to Serial Comm interface, doesn't return until the TX
//! buffer is empty
//!
//! \return none
//
// *****************************************************************************
void TI_SerialComm_TxByte(uint8_t Data)
{
    hal_uart_TxByte(Data);
}

bool TI_SerialComm_RxPeekByte(uint8_t *byte)
{
    return hal_uart_RxPeekByte(byte);
}


//*****************************************************************************
//
//! Convert a nibble to hex value in ASCII
//!
//! \param nibble     expected to be 0x00-0x0F
//!
//! The nibble will be converted to its ASCII value (i.e. 0 = '0', 0xF = 'F')
//!
//! \return Converted ASCII value of nibble
//
// *****************************************************************************
uint8_t nibble_to_hex(uint8_t nibble)
{
    uint8_t ret = nibble;
    if(nibble < 10)
    {
        ret += '0';
    }
    else
    {
        ret += 'A' - 10;
    }

    return(ret);
}


//*****************************************************************************
//
//! Convert a nibble to hex value in ASCII
//!
//! \param nibble     expected to be 0x00-0x0F
//!
//! The nibble will be converted to its ASCII value (i.e. 0 = '0', 0xF = 'F')
//!
//! \return Converted ASCII value of nibble
//
// *****************************************************************************
int8_t char_to_hexint(uint8_t character)
{
    if((character >= '0') && (character <= '9'))
    {
        return (character - '0');
    }
    else if((character >= 'A') && (character <= 'F'))
    {
        return (character - 'A' + 10);
    }
    else if((character >= 'a') && (character <= 'f'))
    {
        return (character - 'a' + 10);
    }
    else
    {
        return(-1);
    }
}

//*****************************************************************************
//
//! Transmit an hexadecimal byte as ASCII
//!
//! \param Data  Byte to be transmitted
//!
//! Converts a byte to ASCII and sends it to Serial interface
//!
//! \return none
//
// *****************************************************************************
void TI_SerialComm_TxHexByte(uint8_t Data)
{
    TI_SerialComm_TxByte(nibble_to_hex(Data >> 4));
    TI_SerialComm_TxByte(nibble_to_hex(Data & 0x0F));
}

//*****************************************************************************
//
//! Transmits a String
//!
//! \param Data  Pointer to string to be transmitted (Null terminated)
//!
//! Sends a string to the serial interface until it finds a Null char (0x00)
//!
//! \return none
//
// *****************************************************************************
void TI_SerialComm_TxString(uint8_t *Data)
{
    while(*Data != 0x00)
    {
        // Load data onto buffer
        TI_SerialComm_TxByte(*Data++);
    }
}

//*****************************************************************************
//
//! Transmits an Array
//!
//! \param Data  Pointer to Array to be transmitted
//! \param Len   Number of bytes to send from the array
//!
//! Sends an array to the serial interface from Byte[0] to Byte[len-1]
//!
//! \return none
//
// *****************************************************************************
void TI_SerialComm_TxArray(uint8_t *Data, uint16_t len)
{
    uint16_t i;

    for (i=0; i < len; i++)
    {
        TI_SerialComm_TxByte(Data[i]);
    }
}

//*****************************************************************************
//
//! Transmits a HEX Array
//!
//! \param Data  Pointer to Array to be transmitted
//! \param Len   Number of bytes to send from the array
//!
//! Sends an array to the serial interface from Byte[0] to Byte[len-1]
//!
//! \return none
//
// *****************************************************************************
void TI_SerialComm_TxHexArray(uint8_t *Data, uint16_t len)
{
    uint16_t i;

    for (i=0; i < len; i++)
    {
        TI_SerialComm_TxHexByte(Data[i]);
    }
}

//*****************************************************************************
//
//! Transmits a response packet
//!
//! \param RespData Response to PC
//! \param RespLen  Lenght of response from SMBus slave
//!
//! This function transmits a packet as expected by the PC application.
//!
//! \return none
//
// *****************************************************************************
void TI_SerialComm_TxPacket(t_SerialComm_Packet *TxPacket)
{
    uint16_t len = 0;
    t_SC_Payload_list_item * item;

    item = TxPacket->PayloadFirst;

    while (item != NULL)
    {
        len += item->Len;
        item = item->Next_item;
    }

    TI_SerialComm_TxByte(SERIALCOMM_HEADER);    // Send Header
    TI_SerialComm_TxByte(TxPacket->Cmd);        // Send Command
    TI_SerialComm_TxByte((len&0xFF00)>>8);      // Send 1st byte of Len (MSB)
    TI_SerialComm_TxByte(len&0x00FF);           // Send 2nd byte of Len (LSB)
    item = TxPacket->PayloadFirst;
    while ((item != NULL) && (len > 0))
    {
        TI_SerialComm_TxArray(item->PayloadPtr, item->Len);   // Send Response data
        item = item->Next_item;
    }
}


//*****************************************************************************
//
//! Event called when new byte is received from Serial Comm
//!
//! \param Data  Received byte
//!
//! \return true to wake-up CPU in ISR, false to stay in LPM
//
// *****************************************************************************
bool hal_uart_DataRx_Event(uint8_t Data)
{
    uint8_t error_code = SERIALCOMM_NO_ERROR;
    bool ret = false; // If true, MCU will wake-up from LPM in ISR

    switch(s_SerialComm.eState)
    {
        case SerialComm_WaitHdr:
            if (Data == SERIALCOMM_HEADER)
            {
                s_SerialComm.eState = SerialComm_WaitCMD;
            }
          break;
        case SerialComm_WaitCMD:
            // Store command in buffer
            s_SerialComm.pRXPacket->Cmd = Data;
            // Command must be valid, and come from PC
            if ( (Data >= 0x80) && (Data < CMD_FROM_PC_END) )
            {
                s_SerialComm.eState = SerialComm_WaitLen1;
            }
            else
            {
                // Invalid command
                s_SerialComm.eState = SerialComm_Error;
                error_code = SERIALCOMM_ERR_INVALID_CMD;
            }
        break;
        case SerialComm_WaitLen1:
            // Get 1st byte of Len (MSB)
            s_SerialComm.pRXPacket->PayloadFirst->Len = (Data << 8);
            s_SerialComm.eState = SerialComm_WaitLen2;
        break;
        case SerialComm_WaitLen2:
            // Get 2nd byte of Len (LSB)
            s_SerialComm.pRXPacket->PayloadFirst->Len += Data;
            if (s_SerialComm.pRXPacket->PayloadFirst->Len > s_SerialComm.RX_MaxLen)
            {
                // Invalid Len
                s_SerialComm.eState = SerialComm_Error;
                error_code = SERIALCOMM_ERR_INVALID_PACKETLEN;
            }
            else if (s_SerialComm.pRXPacket->PayloadFirst->Len == 0x00)
            {
                if (pRxComp_Handler != NULL)
                {
                    pRxComp_Handler(s_SerialComm.pRXPacket);
                }
                s_SerialComm.eState = SerialComm_CMDReceived;
                ret = true; // Wake-up CPU
            }
            else
            {
                s_SerialComm.eState = SerialComm_RxData;
            }
        break;
        case SerialComm_RxData:
          // Get the data
          if (s_SerialComm.RxIndex < s_SerialComm.pRXPacket->PayloadFirst->Len)
          {
                s_SerialComm.pRXPacket->PayloadFirst->PayloadPtr[s_SerialComm.RxIndex++] = Data;
          }

          if (s_SerialComm.RxIndex >= s_SerialComm.pRXPacket->PayloadFirst->Len)
          {
                if (pRxComp_Handler != NULL)
                {
                    pRxComp_Handler(s_SerialComm.pRXPacket);
                }
                s_SerialComm.eState = SerialComm_CMDReceived;
                ret = true; // Wake-up CPU
          }
        break;
        default:
          ret = true; // Wake-up CPU
        break;
    }

    // Check for errors
    if (s_SerialComm.eState == SerialComm_Error)
    {
        if (pError_Handler != NULL)
        {
            // Call error handler
            pError_Handler(error_code);
            ret = true; // Wake-up CPU
        }
    }

    return ret;

}




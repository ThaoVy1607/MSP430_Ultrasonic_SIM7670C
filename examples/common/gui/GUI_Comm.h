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
#ifndef __GUI_COMM_H__
#define __GUI_COMM_H__

//
// Include files
//
#include <stdint.h>
#include <stdbool.h>
#include "TI_SerialComm.h"
//
// Macros and definitions
//
// [ICReduced GUI timeout
#define GUI_TIMEOUT     (2000000)
typedef enum _USS_SW_GUI_RX_COMM_return_code_
{
  // Reserved errors 0-29 for configuration errors
  USS_SW_GUI_RX_COMM_return_code_DUMMY = 0,
  USS_SW_GUI_RX_COMM_return_code_NEW_PKT_AVAILABLE = 1,
  USS_SW_GUI_RX_COMM_return_code_NO_NEW_PKT = 2,
  USS_SW_GUI_RX_COMM_return_code_WAKEByte_FALSE = 3,
  USS_SW_GUI_RX_COMM_return_code_RxPeekByte_FALSE = 4
} USS_SW_GUI_RX_COMM_return_code;

typedef struct 
{
    bool        bActive;
    bool        bParam_Update;
    bool        bUps_Dns;
    bool        bValid;
    uint32_t    u32TransducerFreq;
    uint16_t    u16GapStartCapt;
    uint16_t    u16NumPulses;
    uint16_t    u16GapUpsDns;
    uint16_t    u16GapDnsUps;
    uint8_t     u8GainControl;
    float       fAreaConst;
    uint16_t    u16CrystalFreq_KHz;
    uint16_t    u16AdcSamplingFreq_MHz;
    uint16_t    u16SigSamplingFreq_KHz;
    uint8_t     u8AdcOverSampleRate;
    int16_t     i16DltOffset_psec;
    int16_t     i16AbsTOFAdditionalDelay_nsec;
    uint16_t    u16CaptureDuration_usec;
    uint8_t     u8UserSwitchParam;
    uint16_t    u16Param1;
    uint16_t    u16Param2;
    uint16_t    u16Param3;
    uint16_t    u16Param4;
    uint16_t    u16Param5;
    uint16_t    u16Param6;
    uint16_t    u16Param7;
    uint16_t    u16Param8;
    uint16_t    u16Param9;
    uint16_t    u16Param10;

    uint16_t    u16ARRAY_SIZE;
    uint16_t    u16UPSAMP_FACTOR;       // unused
    uint16_t    u16FRAC_DELAY;          // unused
    uint16_t    u16NUM_SAMPLES;         // unused
    uint16_t    u16GAPSEQUENCE_ACLK;    // unused
    uint32_t    u32PLLFREQ_HZ;
}t_User_Config_Legacy;

typedef struct
{
    bool        bActive_;
    bool        bParam_Update_;
    bool        bUps_Dns_;
    bool        bValid_;
    uint32_t    u32TransducerFreq_TenHz_;    // u32TransducerFreq
    uint16_t    u16GapStartCapt_XTCycles_;   // u16GapStartCapt
    uint16_t    u16NumPulses_;
    uint16_t    u16GapUpsDns_XTCycles_;      // u16GapUpsDns
    uint16_t    u16GapDnsUps_4096Cycles_;    // u16GapDnsUps
    uint8_t     u8GainControl_;
    float       fVolumeScaleFactor_u3_;      // fAreaConst
    uint16_t    u16CrystalFreq_KHz_;
    uint16_t    u16AdcSamplingFreq_KHz_;     // u16AdcSamplingFreq_MHz
    uint16_t    u16SigSamplingFreq_KHz_;
    uint8_t     u8AdcOverSampleRate_;
    int16_t     i16DltOffset_psec_;
    int16_t     i16AbsTOFAdditionalDelay_nsec_;
    uint16_t    u16CaptureDuration_usec_;
    uint8_t     u8UserSwitchParam_;
    uint16_t    u16Cmd_;
    uint16_t    u16Param2_;
    uint16_t    u16StartPPG_10ns_;      // u16Param3
    uint16_t    u16TurnOnADC_10ns_;     // u16Param4
    uint16_t    u16StartPGAInBias_10ns_;// u16Param5
    uint16_t    u16PGAGain_;            //u16Param6;
    uint16_t    u16UssXTALSettlingUsec_;	//u16Param7_;
    uint16_t    u16Param8_;
    uint16_t    u16Param9_;
    uint16_t    u16Param10_;
    uint16_t    u16ARRAY_SIZE_;
    uint16_t    u16unused1_;
    uint16_t    u16unused2_;
    uint16_t    u16unused3_;
    uint16_t    u16unused4_;
    uint32_t    u32PLLFREQ_HZ_;
}t_User_Config_Banshee;

typedef union
{
    t_User_Config_Legacy;
    t_User_Config_Banshee  FR604xComm;
}t_User_Config;

typedef struct 
{
    float       fDeltaTOF;
    float       fAvg_UPS_TotalTOF;
    float       fAvg_DNS_TotalTOF;
    float       fDelta_Std;
    float       fVolume;
    int16_t     iMax_UPS_DNS;
}t_Calc_Results;



//
// Functions 
// 
extern void GUI_Comm_Init(void);
extern void GUI_Comm_RxEnable(void);
extern void GUI_Comm_RxDisable(void);
extern void GUI_Comm_Request_Update_Params(void);
extern void GUI_Comm_Request_UserSelection(void);
extern void GUI_Comm_Send_UPS_DNS(void);
extern void GUI_Comm_Send_Results(void);
extern bool GUI_Comm_IsNewPacketAvailable(uint8_t *receivedbyte);

t_User_Config * GetUserConfigPtr(void);
t_Calc_Results * GetCalcResultPtr(void);


#endif //__GUI_COMM_H__

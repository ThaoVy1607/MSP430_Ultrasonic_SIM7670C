//
//Include Files
//
#include <msp430.h>
#include "TI_SerialComm.h"
#include "GUI_Comm.h"
#include "GUI_meter_config.h"
#include "ussSwLib.h"
#include "stdint.h"   // needed for C99 data types
#include "stdio.h"   // needed for NULL
#include "hmi.h"
//
// Global variables
//

t_User_Config sUser_Config =
{
    .bActive                    = false,
    .bParam_Update              = false,
    .bUps_Dns                   = false,
    .bValid                     = false,
};


t_Calc_Results  sCalc_Results;

#define MAX_RX_SIZE 3000
#define MAX_TX_SIZE 32
uint8_t RX_Buffer[MAX_RX_SIZE];
uint8_t TX_Buffer[MAX_RX_SIZE];
t_SC_Payload_list_item  Rx_list_item;
t_SerialComm_Packet Rx_Packet;

static void SC_Error_Callback(uint8_t Error);
static void SC_PacketComplete_Callback(t_SerialComm_Packet *RxPacket);
static void TX_Command(uint8_t cmd);
//
// Local variables
//

void GUI_Comm_Init(void)
{
    TI_SerialComm_Init();
    TI_SerialComm_SetErrorHandler(SC_Error_Callback);
    TI_SerialComm_PacketCompleteHandler(SC_PacketComplete_Callback);
    // Initialize the RX list
    Rx_list_item.Next_item = NULL;
    Rx_list_item.PayloadPtr = RX_Buffer;
    Rx_Packet.PayloadFirst = &Rx_list_item;
}

void GUI_Comm_RxEnable(void)
{
    TI_SerialComm_RxEnable();
}

void GUI_Comm_RxDisable(void)
{
    TI_SerialComm_RxDisable();
}

bool GUI_Comm_IsNewPacketAvailable(uint8_t *receivedbyte)
{
    uint8_t byte;
    
    if (TI_SerialComm_RxPeekByte(&byte) == true)
    {
        *receivedbyte = byte;
        return true;
    }
    else
    {
      return false;
    }
    
}

void GUI_Comm_Request_Update_Params(void)
{
    TI_SerialComm_StartPacketReception(&Rx_Packet, MAX_RX_SIZE);
    // Request Default configuration
    TX_Command (CMD_TO_PC_REQ_CONFIG);
    if(TI_SerialComm_WaitForRXPacket_wTimeout(GUI_TIMEOUT))
    {
        HMI_UpdateParamsFromGUI();
    }
    
}



void GUI_Comm_Request_UserSelection(void)
{
    TI_SerialComm_StartPacketReception(&Rx_Packet, MAX_RX_SIZE);
    // Request User Update configuration
    TX_Command (CMD_TO_PC_REQ_USER_SEL);
    TI_SerialComm_WaitForRXPacket_wTimeout(GUI_TIMEOUT);
}

void GUI_Comm_Send_UPS_DNS(void)
{
    TX_Command (CMD_TO_PC_UPS_DNS);
}

void GUI_Comm_Send_Results(void)
{
     // Send TOF
    TX_Command (CMD_TO_PC_TOF);
     // Send STD Data
    TX_Command (CMD_TO_PC_STD_DATA);
    // Send Absolute TOF
    TX_Command (CMD_TO_PC_ABS_TOF);
     // Send Volume
    TX_Command (CMD_TO_PC_VOL);
}

t_User_Config * GetUserConfigPtr(void)
{
    return &sUser_Config;
}

t_Calc_Results * GetCalcResultPtr(void)
{
    return &sCalc_Results;
}

static void SC_Error_Callback(uint8_t Error)
 {
    // Restart Packet reception on error
    TI_SerialComm_StartPacketReception(&Rx_Packet, MAX_RX_SIZE);
 }
 
static void SC_PacketComplete_Callback(t_SerialComm_Packet *RxPacket)
 {
    t_SC_Payload_list_item *item = RxPacket->PayloadFirst;
    uint32_t temp;

    
    switch (RxPacket->Cmd)
    {
        case CMD_FROM_PC_USER_SEL:
          if (item->Len == 2)
          {
                sUser_Config.bActive = item->PayloadPtr[0] & BIT0;
                sUser_Config.bParam_Update = (item->PayloadPtr[0] & BIT1)>>1;
                sUser_Config.bUps_Dns = (item->PayloadPtr[0] & BIT2)>>2;
                TX_Command (CMD_TO_PC_USER_SEL_OK);
          }
        break;
        
        //Added for array support
        case CMD_FROM_PC_ARRAY_DATA:
            sUser_Config.bValid = false;
            sUser_Config.u32TransducerFreq = ((uint32_t) item->PayloadPtr[3]) + ((uint32_t) item->PayloadPtr[2] << 8) + ((uint32_t) item->PayloadPtr[1] << 16) + ((uint32_t) item->PayloadPtr[0] << 24);
            sUser_Config.u16GapStartCapt = item->PayloadPtr[5] + (item->PayloadPtr[4] << 8);
            sUser_Config.u16NumPulses = item->PayloadPtr[7] + (item->PayloadPtr[6] << 8);
            // todo: Change u16GapUpsDns so it's based on ACLK
            temp = item->PayloadPtr[9] + (item->PayloadPtr[8] << 8);
            //sUser_Config.u16GapUpsDns = (uint32_t)  temp * (uint32_t) 4096 / (uint32_t) SAMPLING_MCLK_FREQ;
            sUser_Config.u16GapUpsDns = (uint16_t) temp;
            
            sUser_Config.u16GapDnsUps = item->PayloadPtr[11] + (item->PayloadPtr[10] << 8);
            sUser_Config.u8GainControl = item->PayloadPtr[12];
            sUser_Config.u8AdcOverSampleRate = item->PayloadPtr[13];  //u8AdcSamplePulse (ADC_SAMPLE_PULSE) in GUI
            temp = ((uint32_t)item->PayloadPtr[17]) + ((uint32_t) item->PayloadPtr[16] << 8) + ((uint32_t) item->PayloadPtr[15] << 16) + ((uint32_t) item->PayloadPtr[14] << 24);
            sUser_Config.fAreaConst = *(float *) (&temp);

            sUser_Config.u16CrystalFreq_KHz = item->PayloadPtr[19] + (item->PayloadPtr[18] << 8);
            sUser_Config.u16AdcSamplingFreq_MHz = item->PayloadPtr[21] + (item->PayloadPtr[20] << 8);
            sUser_Config.u16SigSamplingFreq_KHz = item->PayloadPtr[23] + (item->PayloadPtr[22] << 8);
            //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // NOTE: Legacy GUI: u16SigSamplingFreq_KHz is used for actual Signal sampling
            //          Frequency. PLL Output Frequency is derived from this and the 
            //          u8AdcOverSampleRate.
            //  PLL Output Frequency = u16SigSamplingFreq_KHz*u8AdcOverSampleRate;
            //          ADC_SAMPLING_FREQ cannot be set to more than 200 KHZ in the GUI
            //          ADC_SAMPLING_FREQ & SIG_SAMPLING_FREQ usage in GUI are swapped !!
            //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            sUser_Config.i16DltOffset_psec = item->PayloadPtr[25] + (item->PayloadPtr[24] << 8);
            sUser_Config.i16AbsTOFAdditionalDelay_nsec = item->PayloadPtr[27] + (item->PayloadPtr[26] << 8);
            //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // TEMP: For Legacy GUI version 1.00.02.01, it actually inputs # of samples
            //  JIRA: MSPGASLIB-8 Bug Filed.
            // Correct this in later usage with later versions of GUI
            sUser_Config.u16CaptureDuration_usec = item->PayloadPtr[29] + (item->PayloadPtr[28] << 8);
            //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            sUser_Config.u8UserSwitchParam = item->PayloadPtr[30];
            sUser_Config.u16Param1 = item->PayloadPtr[32] + (item->PayloadPtr[31] << 8);
            sUser_Config.u16Param2 = item->PayloadPtr[34] + (item->PayloadPtr[33] << 8);
            sUser_Config.u16Param3 = item->PayloadPtr[36] + (item->PayloadPtr[35] << 8);
            sUser_Config.u16Param4 = item->PayloadPtr[38] + (item->PayloadPtr[37] << 8);
            sUser_Config.u16Param5 = item->PayloadPtr[40] + (item->PayloadPtr[39] << 8);
            sUser_Config.u16Param6 = item->PayloadPtr[42] + (item->PayloadPtr[41] << 8);
            sUser_Config.u16Param7 = item->PayloadPtr[44] + (item->PayloadPtr[43] << 8);
            sUser_Config.u16Param8 = item->PayloadPtr[46] + (item->PayloadPtr[45] << 8);
            sUser_Config.u16Param9 = item->PayloadPtr[48] + (item->PayloadPtr[47] << 8);
            sUser_Config.u16Param10= item->PayloadPtr[50] + (item->PayloadPtr[49] << 8);
            
            //  Derived: PLL Output Frequency = u16SigSamplingFreq_KHz*u8AdcOverSampleRate;
            sUser_Config.u32PLLFREQ_HZ = ((uint32_t) sUser_Config.u16SigSamplingFreq_KHz * 1000 *
                                          (uint32_t) sUser_Config.u8AdcOverSampleRate);

            //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            sUser_Config.u16ARRAY_SIZE	       =  (uint16_t)((float)sUser_Config.u16CaptureDuration_usec*(float)sUser_Config.u16SigSamplingFreq_KHz*(float)1.0e-3); //1e-6*1e3

            sUser_Config.bValid = true;
            
            TX_Command (CMD_TO_PC_PARAM_OK);
///////////////////////////////////////////////                                                                 
        break;        
        case CMD_FROM_PC_PARAM_CONFIG: //axs: In the current GUI (4/12/2016 this is not being sent. It is combined with CMD_FROM_PC_ARRAY_DATA)
          if (item->Len == 18)
          {
                TX_Command (CMD_TO_PC_PARAM_OK);
          }
        break;
      
    }
 }

static void TX_Command(uint8_t cmd)
{
    t_SC_Payload_list_item  Tx_first_item, Tx_2nd_item;
    t_SerialComm_Packet Tx_Packet;

    Tx_Packet.Cmd = cmd;
    switch (cmd)
    {
        case CMD_TO_PC_REQ_CONFIG:
        case CMD_TO_PC_REQ_USER_SEL:
        case CMD_TO_PC_USER_SEL_OK:
        case CMD_TO_PC_PARAM_OK:
        Tx_first_item.PayloadPtr = NULL;
        Tx_first_item.Len = 0x00;
        Tx_first_item.Next_item = NULL;
        Tx_Packet.PayloadFirst = &Tx_first_item;
        TI_SerialComm_TxPacket(&Tx_Packet);
        break;
        case CMD_TO_PC_TOF:
            Tx_first_item.PayloadPtr = (uint8_t *) &sCalc_Results.fDeltaTOF;
            Tx_first_item.Len = sizeof(sCalc_Results.fDeltaTOF);
            Tx_first_item.Next_item = NULL;
            Tx_Packet.PayloadFirst =  &Tx_first_item;
            TI_SerialComm_TxPacket(&Tx_Packet);
        break;
        case CMD_TO_PC_STD_DATA:
            Tx_first_item.Next_item = NULL;
            Tx_first_item.PayloadPtr = (uint8_t *) &sCalc_Results.fDelta_Std;
            Tx_first_item.Len = sizeof(sCalc_Results.fDelta_Std);
            Tx_first_item.Next_item = NULL;
            Tx_Packet.PayloadFirst =  &Tx_first_item;
            TI_SerialComm_TxPacket(&Tx_Packet);
        break;
        case CMD_TO_PC_ABS_TOF:
            Tx_first_item.PayloadPtr = (uint8_t *) &sCalc_Results.fAvg_UPS_TotalTOF;
            Tx_first_item.Len = sizeof(sCalc_Results.fAvg_UPS_TotalTOF);
            Tx_first_item.Next_item = &Tx_2nd_item;
            Tx_2nd_item.PayloadPtr = (uint8_t *) &sCalc_Results.fAvg_DNS_TotalTOF;
            Tx_2nd_item.Len = sizeof(sCalc_Results.fAvg_DNS_TotalTOF);
            Tx_2nd_item.Next_item = NULL;
            Tx_Packet.PayloadFirst =  &Tx_first_item;
            TI_SerialComm_TxPacket(&Tx_Packet);
        break;
        case CMD_TO_PC_VOL:
            Tx_first_item.PayloadPtr = (uint8_t *) &sCalc_Results.fVolume;
            Tx_first_item.Len = sizeof(sCalc_Results.fVolume);
            Tx_first_item.Next_item = NULL;
            Tx_Packet.PayloadFirst =  &Tx_first_item;
            TI_SerialComm_TxPacket(&Tx_Packet);
        break;
        case CMD_TO_PC_UPS_DNS:
          Tx_first_item.PayloadPtr = (uint8_t *) USS_getUPSPtr();
            Tx_first_item.Len = (sUser_Config.u16ARRAY_SIZE)*2;
            Tx_first_item.Next_item = &Tx_2nd_item;
            Tx_2nd_item.PayloadPtr 
              = (uint8_t *) USS_getDNSPtr();
            Tx_2nd_item.Len = (sUser_Config.u16ARRAY_SIZE)*2;
            Tx_2nd_item.Next_item = NULL;
            Tx_Packet.PayloadFirst =  &Tx_first_item;
            TI_SerialComm_TxPacket(&Tx_Packet);          
        break;
    }
}


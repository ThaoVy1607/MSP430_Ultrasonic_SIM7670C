#include "USSLibGUIApp.h"
#include "USSLibGUIApp/fr6047_USS_app/lcd_statemachineDC/lcd_statemachine.h"
#include "USSLibGUIApp/fr6047_USS_app/resultsDC/results.h"
#include "USS_Config/USS_userConfig.h"
#include "hal.h"
#include "hmiDC/hmi.h"
#include "mathematicsDC/mathematics.h"
#include <stdint.h>

// #include "USS/ussSwLib/ussSwLib.h"
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(app_version, ".version")
#pragma RETAIN(app_version)
const uint8_t app_version[8] = {
    0x02, 0x40, 0x00, 0x00, __EXTRA_VERSION__, __EVM430_ID__, __EVM430_VER__};
#else // __TI_COMPILER_VERSION__

#pragma location = "VERSION"
__root const uint8_t app_version[8] = {
    0x02, 0x40, 0x00, 0x00, __EXTRA_VERSION__, __EVM430_ID__, __EVM430_VER__};
#endif
extern RESULTS_MEASUREMENT_DATA g_ResultsOfLastMeasurement;

/*** FUNCTIONS ***/

//*****************************************************************************
//
//! In this software main will call a few initialization routines and then it
//!  will jump to the main loop for the Design Center
//!
//! \return none
//
// *****************************************************************************

void main(void) {
  uint16_t reset_source = 0x00;

  // Initializes the basic functionality of the system
  hal_system_Init();
  InitTimerMillis();
  // Initialize interfaces to user including buttons, LCD, GUI
  HMI_Init();

  // Verify the reset source and log/report if there was an error
  reset_source = hal_system_GetResetSource();
  if (reset_source != 0x00) {
    USSLibGUIApp_send_error(COMMAND_HANDLER_ERROR_FAULT_RESET_ERROR,
                            reset_source);
  }

  __enable_interrupt();

  // Initialize the USS Lib
  USSLibGUIApp_Init();
  USSLibGUIApp_Engine();
  // WDTCTL = WDTPW | WDTHOLD; // Dừng watchdog

  // This is the Design Centers Main Loop
//   while (1) {
//     hal_lcd_turnonLCD();

//     hal_lcd_showChar('5', HAL_LCD_DIGIT_5);
//     hal_lcd_showChar('6', HAL_LCD_DIGIT_6);
//     hal_lcd_showChar('7', HAL_LCD_DIGIT_7);
//     hal_lcd_showChar('8', HAL_LCD_DIGIT_8);
//     hal_lcd_showChar('9', HAL_LCD_DIGIT_9);
//     hal_lcd_showChar('0', HAL_LCD_DIGIT_10);
//     // uint8_t data1 = (millis() >> 24) & 0xFF;
//     // uint8_t data2 = (millis() >> 16) & 0xFF;
//     // uint8_t data3 = (millis() >> 8) & 0xFF;
//     // uint8_t data4 = (millis() >> 0) & 0xFF;

//     // // Send_SendATCommands(50.5);
//     // hal_uart_TxByte(data1);
//     // hal_uart_TxByte(data2);
//     // hal_uart_TxByte(data3);
//     // hal_uart_TxByte(data4);
//     hal_system_WatchdogFeed();
//     // __delay_cycles(8000000);
//   }
}

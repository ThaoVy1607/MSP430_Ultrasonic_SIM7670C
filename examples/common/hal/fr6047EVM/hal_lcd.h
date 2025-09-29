#ifndef _MSP430FR6047_HAL_LCD_H_
#define _MSP430FR6047_HAL_LCD_H_

/*! Enables/disables the external resistor ladder */
#define LCD_USE_RES_LADDER
/*! Enables/disables the internal charge pump */
//#define LCD_USE_CHARGEPUMP

/*! Enables/disables the LCD */
#define ENABLE_LCD
/*! Maximum number of digits in LCD */
#define HAL_LCD_DIGIT_MAX  10
/*! Size of LCD Memory map */
#define HAL_LCD_MEMMAP_SIZE 30

/*! Start Segment used for each one of the digits */
// typedef enum
// {
//     HAL_LCD_DIGIT_1 = LCD_C_SEGMENT_LINE_0, /* Digit 1 in LCDMEM1 */
//     HAL_LCD_DIGIT_2 = LCD_C_SEGMENT_LINE_4, /* Digit 2 in LCDMEM3 */
//     HAL_LCD_DIGIT_3 = LCD_C_SEGMENT_LINE_8, /* Digit 3 in LCDMEM5 */
//     HAL_LCD_DIGIT_4 = LCD_C_SEGMENT_LINE_12, /* Digit 4 in LCDMEM7 */
//     HAL_LCD_DIGIT_5 = LCD_C_SEGMENT_LINE_16, /* Digit 5 in LCDMEM9 */
//     HAL_LCD_DIGIT_6 = LCD_C_SEGMENT_LINE_23,/* Digit 6 in LCDMEM12 */
// }HAL_LCD_DIGIT_t;

typedef enum
{
    HAL_LCD_DIGIT_1 = LCD_C_SEGMENT_LINE_0, /* Digit 1 in LCDMEM1 */
    HAL_LCD_DIGIT_2 = LCD_C_SEGMENT_LINE_2, /* Digit 2 in LCDMEM3 */
    HAL_LCD_DIGIT_3 = LCD_C_SEGMENT_LINE_4, /* Digit 3 in LCDMEM5 */
    HAL_LCD_DIGIT_4 = LCD_C_SEGMENT_LINE_6, /* Digit 4 in LCDMEM7 */
    HAL_LCD_DIGIT_5 = LCD_C_SEGMENT_LINE_11, /* Digit 5 in LCDMEM9 */
    HAL_LCD_DIGIT_6 = LCD_C_SEGMENT_LINE_13,/* Digit 6 in LCDMEM12 */
    HAL_LCD_DIGIT_7 = LCD_C_SEGMENT_LINE_15,/* Digit 6 in LCDMEM12 */
    HAL_LCD_DIGIT_8 = LCD_C_SEGMENT_LINE_19,/* Digit 6 in LCDMEM12 */
    HAL_LCD_DIGIT_9 = LCD_C_SEGMENT_LINE_21,/* Digit 6 in LCDMEM12 */
    HAL_LCD_DIGIT_10 = LCD_C_SEGMENT_LINE_23,/* Digit 6 in LCDMEM12 */
}HAL_LCD_DIGIT_t;

/*! Enumeration of symbols available in LCD */
typedef enum
{
    HAL_LCD_SYMBOL_MINUS = 0x00,    /* Symbol for '-' */
    HAL_LCD_SYMBOL_WIRELESS,        /* Symbol for Wireless Antenna */
    HAL_LCD_SYMBOL_COLON_HR,        /* Symbol for ':' for Hours */
    HAL_LCD_SYMBOL_COLON_MIN,       /* Symbol for ':' for Minutes */
    HAL_LCD_SYMBOL_DEGREES,         /* Symbol for Degrees */
    HAL_LCD_SYMBOL_WARNING,         /* Symbol for '!' */
    HAL_LCD_SYMBOL_TM,              /* Symbol for (R) */
    HAL_LCD_SYMBOL_HEART,           /* Symbol for a heart */
    HAL_LCD_SYMBOL_CLOCK,           /* Symbol for a clock */
    HAL_LCD_SYMBOL_WIRELESS_RX,     /* Symbol for Wireless RX */
    HAL_LCD_SYMBOL_WIRELESS_TX,     /* Symbol for Wireless TX */
    HAL_LCD_SYMBOL_BATTERY_TIP,     /* Symbol for battery tip */
    HAL_LCD_SYMBOL_BATTERY_CASE,    /* Symbol for battery case*/
    HAL_LCD_SYMBOL_BATTERY_1,       /* Symbol for battery bar 1 */
    HAL_LCD_SYMBOL_BATTERY_2,       /* Symbol for battery bar 2 */
    HAL_LCD_SYMBOL_BATTERY_3,       /* Symbol for battery bar 3*/
    HAL_LCD_SYMBOL_BATTERY_4,       /* Symbol for battery bar 4*/
    HAL_LCD_SYMBOL_BATTERY_5,       /* Symbol for battery bar 5 */
    HAL_LCD_SYMBOL_BATTERY_6,       /* Symbol for battery bar 6 */
}HAL_LCD_SYMBOL_t;


//*****************************************************************************
//
//! Initialization of LCD module and its pins
//!
//! \return none
//
// *****************************************************************************
extern void hal_lcd_Init(void);

//*****************************************************************************
//
//! Turn off LCD
//!
//! \return none
//
// *****************************************************************************
extern void hal_lcd_turnoffLCD(void);

//*****************************************************************************
//
//! Turn on the LCD
//!
//! \return none
//
// *****************************************************************************
extern void hal_lcd_turnonLCD(void);

//*****************************************************************************
//
//! Displays a scrolling text through 6 position display
//!
//! \param msg      Pointer to the message (must have NULL termination)
//! \param delay    Software delay for scrolling each character
//!
//! \return none
//
// *****************************************************************************
extern void hal_lcd_displayScrollText(char *msg, uint16_t delay);

//*****************************************************************************
//
//! Show a character (digit, capital letter or space) in a particular position
//! of the LCD
//!
//! \param c        Character to display in ASCII (digit, capital letter or space)
//! \param  position Position where character will be displayed (HAL_LCD_DIGIT_x)
//!
//! \return none
//
// *****************************************************************************
extern void hal_lcd_showChar(char c, HAL_LCD_DIGIT_t position);

//*****************************************************************************
//
//! Show a character (digit, capital letter or space) in a particular position
//! of the LCD with option to show a decimal point
//!
//! \param c        Character to display in ASCII (digit, capital letter or space)
//! \param  position Position where character will be displayed (HAL_LCD_DIGIT_x)
//! \param  decPoint true if decimal point is turned on. false if not.
//!
//! \return none
//
// *****************************************************************************
extern void hal_lcd_showCharwPoint(char c,
                                   HAL_LCD_DIGIT_t position,
                                   bool decPoint);

//*****************************************************************************
//
//! Clear the LCD
//!
//! \return none
//
// *****************************************************************************
extern void hal_lcd_clearLCD(void);

//*****************************************************************************
//
//! Turn on a symbol
//!
//! \param symbol Symbol to turn on (HAL_LCD_SYMBOL_t)
//!
//! \return none
//
// *****************************************************************************
extern void hal_lcd_setSymbol(HAL_LCD_SYMBOL_t symbol);

//*****************************************************************************
//
//! Turn off a symbol
//!
//! \param symbol Symbol to turn off (HAL_LCD_SYMBOL_t)
//!
//! \return none
//
// *****************************************************************************
extern void hal_lcd_clearSymbol(HAL_LCD_SYMBOL_t symbol);

//*****************************************************************************
//
//! Display battery symbol and its percentage
//!
//! \param percentage from 0-100. 0 represents empty battery, 100 is full battery
//!
//! \return none
//
// *****************************************************************************
extern void hal_lcd_showBattery(uint8_t percentage);


#endif /* _MSP430FR6047_HAL_LCD_H_ */

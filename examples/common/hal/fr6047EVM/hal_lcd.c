#include "driverlib.h"
#include "hal.h"
#include "string.h"

#ifdef ENABLE_LCD

/*! LCD Memory map. Used to set individual LCD segments */
static uint8_t LCDMemMap[HAL_LCD_MEMMAP_SIZE] = {0};

/*! LCD memory map for numeric digits */
const uint8_t digitTable[10][2] =
    {
        {0x7D, 0x28}, /* "0" LCD segments a+b+c+d+e+f+k+q */
        {0x30, 0x00}, /* "1" */
        {0x6E, 0x00}, /* "2" */
        {0x7A, 0x00}, /* "3" */
        {0x63, 0x00}, /* "4" */
        {0x5B, 0x00}, /* "5" */
        {0x5F, 0x00}, /* "6" */
        {0x70, 0x00}, /* "7" */
        {0x7F, 0x00}, /* "8" */
        {0x7B, 0x00}  /* "9" */

};

const uint8_t digitTable__sy[10][2] =
    {
        {0x7D, 0x28}, /* "0" LCD segments a+b+c+d+e+f+k+q */
        {0x60, 0x00}, /* "1" */
        {0x3E, 0x00}, /* "2" */
        {0x7A, 0x00}, /* "3" */
        {0x63, 0x00}, /* "4" */
        {0x5B, 0x00}, /* "5" */
        {0x5F, 0x00}, /* "6" */
        {0x70, 0x00}, /* "7" */
        {0x7F, 0x00}, /* "8" */
        {0x7B, 0x00}  /* "9" */

};

/*! LCD memory map for uppercase letters */
const uint8_t alphabetBigTable[26][2] =
    {
        {0xBE, 0x60}, /* "0" LCD segments a+b+c+e+f+g+m */
        {0x06, 0xEC}, /* "1" */
        {0x7C, 0x00}, /* "2" */
        {0x5E, 0x00}, /* "3" */
        {0x2C, 0x00}, /* "4" */
        {0xBE, 0x00}, /* "5" */
        {0xFA, 0x00}, /* "6" */
        {0x0E, 0x00}, /* "7" */
        {0XFF, 0x00}, /* "8" */
        {0xDE, 0x00}, /* "9" */
        // {0xDE, 0x00},  /* "K" */
        // {0x1C, 0x00},  /* "L" */
        // {0x6C, 0xA0},  /* "M" */
        // {0x6C, 0x82},  /* "N" */
        // {0xFC, 0x00},  /* "O" */
        // {0xCF, 0x00},  /* "P" */
        // {0xFC, 0x02},  /* "Q" */
        // {0xCF, 0x02},  /* "R" */
        // {0xB7, 0x00},  /* "S" */
        // {0x80, 0x50},  /* "T" */
        // {0x7C, 0x00},  /* "U" */
        // {0x0C, 0x28},  /* "V" */
        // {0x6C, 0x0A},  /* "W" */
        // {0x00, 0xAA},  /* "X" */
        // {0x00, 0xB0},  /* "Y" */
        // {0x90, 0x28}  /* "Z" */

};
const uint8_t alphabetBigTable___[26] =
    {
        // 0xEF,  /* "A" LCD segments a+b+c+e+f+g+m */
        // 0xF1,  /* "B" */
        // 0x9C,  /* "C" */
        // 0xF0,  /* "D" */
        // 0x1F,  /* "E" */
        // 0x8F,  /* "F" */
        // 0xBD,  /* "G" */
        // 0x67,  /* "H" */
        // 0x90,  /* "I" */
        // 0x78,  /* "J" */
        // 0x0E,  /* "K" */
        // 0x1C,  /* "L" */
        // 0x6C,  /* "M" */
        // 0x6C,  /* "N" */
        // 0xFC,  /* "O" */
        // 0xCF,  /* "P" */
        // 0xFC,  /* "Q" */
        // 0xCF,  /* "R" */
        // 0xB7,  /* "S" */
        // 0x80,  /* "T" */
        // 0x7C,  /* "U" */
        // 0x0C,  /* "V" */
        // 0x6C,  /* "W" */
        // 0x00,  /* "X" */
        // 0x00,  /* "Y" */
        // 0x90,   /* "Z" */
        // [0]=0xEB,/*"0"*/
        // [1]=0X60,/*"1"*/
        // [2]=0X2C,/*"2"*/
        // [3]=0XE5,/*"3"*/
        // [4]=0X2C,/*"4"*/
        // [5]=0XAD,/*"5"*/
        // [6]=0X5F,/*"6"*/
        // [7]=0XE0,/*"7"*/
        // [8]=0XEF,/*"8"*/
        // [9]=0XEB,/*"9"*/
};

/*! LCD Table for symbols */
const uint8_t symbolTable[][2] =
    {
        {0x04, LCD_C_SEGMENT_LINE_2},  /* HAL_LCD_SYMBOL_MINUS */
        {0x04, LCD_C_SEGMENT_LINE_10}, /* HAL_LCD_SYMBOL_WIRELESS */
        {0x04, LCD_C_SEGMENT_LINE_6},  /* HAL_LCD_SYMBOL_COLON_HR */
        {0x04, LCD_C_SEGMENT_LINE_14}, /* HAL_LCD_SYMBOL_COLON_MIN */
        {0x04, LCD_C_SEGMENT_LINE_18}, /* HAL_LCD_SYMBOL_DEGREES */
        {0x01, LCD_C_SEGMENT_LINE_20}, /* HAL_LCD_SYMBOL_WARNING */
        {0x02, LCD_C_SEGMENT_LINE_20}, /* HAL_LCD_SYMBOL_TM */
        {0x04, LCD_C_SEGMENT_LINE_20}, /* HAL_LCD_SYMBOL_HEART */
        {0x08, LCD_C_SEGMENT_LINE_20}, /* HAL_LCD_SYMBOL_CLOCK */
        {0x01, LCD_C_SEGMENT_LINE_25}, /* HAL_LCD_SYMBOL_WIRELESS_RX */
        {0x04, LCD_C_SEGMENT_LINE_25}, /* HAL_LCD_SYMBOL_WIRELESS_TX */
        {0x01, LCD_C_SEGMENT_LINE_21}, /* HAL_LCD_SYMBOL_BATTERY_TIP */
        {0x01, LCD_C_SEGMENT_LINE_22}, /* HAL_LCD_SYMBOL_BATTERY_CASE */
        {0x02, LCD_C_SEGMENT_LINE_22}, /* HAL_LCD_SYMBOL_BATTERY_1 */
        {0x02, LCD_C_SEGMENT_LINE_21}, /* HAL_LCD_SYMBOL_BATTERY_2 */
        {0x04, LCD_C_SEGMENT_LINE_22}, /* HAL_LCD_SYMBOL_BATTERY_3 */
        {0x04, LCD_C_SEGMENT_LINE_21}, /* HAL_LCD_SYMBOL_BATTERY_4 */
        {0x08, LCD_C_SEGMENT_LINE_22}, /* HAL_LCD_SYMBOL_BATTERY_5 */
        {0x08, LCD_C_SEGMENT_LINE_21}, /* HAL_LCD_SYMBOL_BATTERY_6 */
};

//
// Local function prototypes
//
static void hal_lcd_updatememory(uint8_t baseposition, uint8_t num_segments);

void hal_lcd_Init(void)
{
    LCD_C_initParam initParams = {0};
    initParams.clockSource = LCD_C_CLOCKSOURCE_ACLK;
    initParams.clockDivider = LCD_C_CLOCKDIVIDER_1;
    initParams.clockPrescalar = LCD_C_CLOCKPRESCALAR_16;
    initParams.muxRate = LCD_C_4_MUX;
    initParams.waveforms = LCD_C_LOW_POWER_WAVEFORMS;
    initParams.segments = LCD_C_SEGMENTS_ENABLED;

    /*
     * Select Port 6
     * Set Pin [4:7] as LCD COM, [1:3] as R03/R13/R23
     */
#ifdef LCD_USE_RES_LADDER
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P6,
        GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3,
        GPIO_TERNARY_MODULE_FUNCTION);
#else
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P6,
        GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7,
        GPIO_TERNARY_MODULE_FUNCTION);
#endif
    LCD_C_init(LCD_C_BASE, &initParams);
    // LCD Operation - VLCD generated internally, V2-V4 generated internally, v5 to ground

    LCD_C_setPinAsLCDFunctionEx(LCD_C_BASE, LCD_C_SEGMENT_LINE_0,
                                LCD_C_SEGMENT_LINE_26);

#ifdef LCD_USE_RES_LADDER
    LCD_C_setVLCDSource(LCD_C_BASE,
                        LCD_C_VLCD_GENERATED_INTERNALLY,
                        LCD_C_V2V3V4_GENERATED_INTERNALLY_SWITCHED_TO_PINS,
                        LCD_C_V5_SOURCED_FROM_R03);
#else
    LCD_C_setVLCDSource(LCD_C_BASE,
                        LCD_C_VLCD_GENERATED_INTERNALLY,
                        LCD_C_V2V3V4_GENERATED_INTERNALLY_NOT_SWITCHED_TO_PINS,
                        LCD_C_V5_VSS);
#endif

#ifdef LCD_USE_CHARGEPUMP
    LCD_C_setVLCDVoltage(LCD_C_BASE,
                         LCD_C_CHARGEPUMP_VOLTAGE_3_02V_OR_2_52VREF);
    // LCD_C_CHARGEPUMP_VOLTAGE_2_60V_OR_2_17VREF);
    // LCD_C_CHARGEPUMP_VOLTAGE_3_44V_OR_2_87VREF);
    // Enable charge pump and select internal reference for it
    LCD_C_enableChargePump(LCD_C_BASE);
    LCD_C_selectChargePumpReference(LCD_C_BASE,
                                    LCD_C_INTERNAL_REFERENCE_VOLTAGE);
#else
    LCD_C_setVLCDVoltage(LCD_C_BASE,
                         LCD_C_CHARGEPUMP_DISABLED);
    LCD_C_disableChargePump(LCD_C_BASE);

#endif

    LCD_C_configChargePump(LCD_C_BASE, LCD_C_SYNCHRONIZATION_ENABLED, 0);

    // Clear LCD memory
    LCD_C_clearMemory(LCD_C_BASE);

    // Turn LCD on
    LCD_C_on(LCD_C_BASE);
}

void hal_lcd_turnoffLCD(void)
{
    int i = 0;
    LCD_C_off(LCD_C_BASE);
    for (i = 0; i < HAL_LCD_MEMMAP_SIZE; i++)
    {
        LCDMemMap[i] = 0;
    }
}

void hal_lcd_turnonLCD(void)
{
    LCD_C_on(LCD_C_BASE);
}
void hal_lcd_clearLCD(void)
{
    LCD_C_clearMemory(LCD_C_BASE);
}

void hal_lcd_showChar(char c, HAL_LCD_DIGIT_t position)
{
    hal_lcd_showCharwPoint(c, position, false);
}

void hal_lcd_showCharwPoint(char c,
                            HAL_LCD_DIGIT_t position,
                            bool decPoint)
{
    uint8_t value;

    // if (position >  HAL_LCD_DIGIT_6)
    // {
    //     return;
    // }

    if (c == ' ' || position == HAL_LCD_DIGIT_1 || position == HAL_LCD_DIGIT_2 || position == HAL_LCD_DIGIT_3 || position == HAL_LCD_DIGIT_4)
    {
        // Display space
        value = 0x00;
    }
    else if (c >= '0' && c <= '9')
    {
        // Display digit
        value = digitTable__sy[c - 48][0];
    }
    // else if (c >= 'A' && c <= 'Z')
    // {
    //     // Display alphabet
    //     value = alphabetBigTable___[c-65];
    // }
    else
    {
        // Turn all segments on if character is not a space, digit, or uppercase letter
        value = 0xFF;
    }

    if (decPoint == true)
    {
        value |= BIT7;
    }
    // The LCDMx position uses all 8 bits to write the character
    LCDMemMap[position] = value & 0x0F;
    LCDMemMap[position + 1] = value >> 4;
    hal_lcd_updatememory(position, 2);
}

// void hal_lcd_showCharwPoint(char c,
//                             HAL_LCD_DIGIT_t position,
//                             bool decPoint)
// {
//     uint8_t value;
//     uint8_t value2;

//     if (position >  HAL_LCD_DIGIT_6)
//     {
//         return;
//     }

//     if (c == ' ')
//     {
//         // Display space
//         value = 0x00;
//         value2 = 0x00;
//     }
//     else if (c >= '0' && c <= '9')
//     {
//         // Display digit
//         value = digitTable[c-48][0];
//         value2 = digitTable[c-48][1];
//     }
//     else if (c >= 'A' && c <= 'Z')
//     {
//         // Display alphabet
//         value = alphabetBigTable[c-65][0];
//         value2 = alphabetBigTable[c-65][1];
//     }
//     else
//     {
//         // Turn all segments on if character is not a space, digit, or uppercase letter
//         value = 0xFF;
//         value2 = 0xFF;
//     }

//     if (decPoint == true)
//     {
//         value2 |= BIT0;
//     }
//     // The LCDMx position uses all 8 bits to write the character
//     LCDMemMap[position] = value&0x0F;
//     LCDMemMap[position+1] = value>>4;
//     // LCDMx+1 uses BIT0 for period & BIT2 for symbols
//     LCDMemMap[position+2] = (LCDMemMap[position+2] & 0x04) | (value2&0x0B);
//     LCDMemMap[position+3] = value2>>4;
//     hal_lcd_updatememory(position, 4);
// }

/*
 * Scrolls input string across LCD screen from left to right
 */
void hal_lcd_displayScrollText(char *msg, uint16_t delay)
{
    uint16_t length = strlen(msg);
    int16_t i;
    int16_t s = 5;
    char buffer[6] = "      ";
    uint16_t count_delay;
    for (i = 0; i < length + 7; i++)
    {
        uint16_t t;
        for (t = 0; t < 6; t++)
        {
            buffer[t] = ' ';
        }
        int16_t j;
        for (j = 0; j < length; j++)
        {
            if (((s + j) >= 0) && ((s + j) < 6))
            {
                buffer[s + j] = msg[j];
            }
        }
        s--;

        hal_lcd_showChar(buffer[0], HAL_LCD_DIGIT_1);
        hal_lcd_showChar(buffer[1], HAL_LCD_DIGIT_2);
        hal_lcd_showChar(buffer[2], HAL_LCD_DIGIT_3);
        hal_lcd_showChar(buffer[3], HAL_LCD_DIGIT_4);
        hal_lcd_showChar(buffer[4], HAL_LCD_DIGIT_5);
        hal_lcd_showChar(buffer[5], HAL_LCD_DIGIT_6);

        for (count_delay = 0; count_delay < delay; count_delay++)
        {
            hal_system_WatchdogFeed();
            __delay_cycles(8000); // ~1ms
        }
    }
}

void hal_lcd_setSymbol(HAL_LCD_SYMBOL_t symbol)
{
    LCDMemMap[symbolTable[symbol][1]] |= symbolTable[symbol][0];
    hal_lcd_updatememory(symbolTable[symbol][1], 1);
}

void hal_lcd_clearSymbol(HAL_LCD_SYMBOL_t symbol)
{
    LCDMemMap[symbolTable[symbol][1]] &= ~symbolTable[symbol][0];
    hal_lcd_updatememory(symbolTable[symbol][1], 1);
}

void hal_lcd_showBattery(uint8_t percentage)
{
    // Clear battery LCD segments
    LCDMemMap[symbolTable[HAL_LCD_SYMBOL_BATTERY_TIP][1]] = 0x00;
    LCDMemMap[symbolTable[HAL_LCD_SYMBOL_BATTERY_CASE][1]] = 0x00;

    // draw battery case
    hal_lcd_setSymbol(HAL_LCD_SYMBOL_BATTERY_TIP);
    hal_lcd_setSymbol(HAL_LCD_SYMBOL_BATTERY_CASE);

    // draw the percentage
    if (percentage >= 15)
    {
        hal_lcd_setSymbol(HAL_LCD_SYMBOL_BATTERY_1);
    }
    if (percentage >= 30)
    {
        hal_lcd_setSymbol(HAL_LCD_SYMBOL_BATTERY_2);
    }
    if (percentage >= 45)
    {
        hal_lcd_setSymbol(HAL_LCD_SYMBOL_BATTERY_3);
    }
    if (percentage >= 60)
    {
        hal_lcd_setSymbol(HAL_LCD_SYMBOL_BATTERY_4);
    }
    if (percentage >= 75)
    {
        hal_lcd_setSymbol(HAL_LCD_SYMBOL_BATTERY_5);
    }
    if (percentage >= 90)
    {
        hal_lcd_setSymbol(HAL_LCD_SYMBOL_BATTERY_6);
    }
}

static void hal_lcd_updatememory(uint8_t baseposition, uint8_t num_segments)
{
    uint8_t i;

    for (i = 0; i < num_segments; i++)
    {
        LCD_C_setMemory(LCD_C_BASE, baseposition + i, LCDMemMap[baseposition + i]);
    }
}

#endif

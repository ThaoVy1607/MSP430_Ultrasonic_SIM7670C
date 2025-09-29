#include "TI_SerialComm.h"
#include "USSLibGUIApp/fr6047_USS_app/lcd_statemachineDC/lcd_statemachine.h"
#include "msp430.h"
#include <resultsDC/results.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

volatile uint32_t millis_counter = 0;

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A_ISR(void) {
  millis_counter++; // Mỗi lần ngắt là 1ms
}

void InitTimerMillis() {
  // Cấu hình CCR0 để ngắt mỗi 1ms (8MHz / 1000 = 8000 chu kỳ)
  Timer_A_clearTimerInterrupt(TIMER_A0_BASE);

  Timer_A_clear(TIMER_A0_BASE);

  Timer_A_initUpModeParam timerParams = {
      .clockSource = TIMER_A_CLOCKSOURCE_SMCLK,
      .clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1, // 8MHz / 1 = 8MHz
      .timerPeriod = 8000 - 1, // 1ms = 8000 chu kỳ
      .timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE,
      .captureCompareInterruptEnable_CCR0_CCIE =
          TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,
      .timerClear = TIMER_A_SKIP_CLEAR,
      .startTimer = true};

  Timer_A_initUpMode(TIMER_A0_BASE, &timerParams);
}

uint32_t millis() { return millis_counter; }

static volatile uint8_t guRxData;

typedef enum {
  STATE_IDLE = 0,
  STATE_AT_TEST,
  STATE_CHECK_SIM,
  STATE_CHECK_SIGNAL,
  STATE_CHECK_NETWORK,
  STATE_SET_APN,
  STATE_ACTIVATE_PDP,
  STATE_GET_IP,
  STATE_HTTP_INIT,
  STATE_SET_CID,
  STATE_SET_URL,
  STATE_SET_CONTENT_TYPE,
  STATE_SET_DATA_LENGTH,
  STATE_SEND_JSON_DATA,
  STATE_HTTP_POST,
  STATE_WAIT_RESPONSE,
  STATE_COMPLETE,
  STATE_ERROR
} http_state_t;

// Cấu trúc quản lý state machine
typedef struct {
  http_state_t current_state;
  uint32_t state_timer;
  uint32_t timeout_ms;
  float data_to_send;
  char json_buffer[64];
  uint8_t retry_count;
  uint8_t max_retries;
} http_state_machine_t;

static http_state_machine_t sm = {.current_state = STATE_IDLE,
                                  .state_timer = 0,
                                  .timeout_ms = 1000, // 5 giây timeout mặc định
                                  .retry_count = 0,
                                  .max_retries = 3};

static bool is_timeout(void) {
  return (millis() - sm.state_timer) > sm.timeout_ms;
}

// Reset timer cho state mới
static void reset_timer(uint32_t timeout_ms) {
  sm.state_timer = millis();
  sm.timeout_ms = timeout_ms;
}

// Chuyển sang state mới
static void change_state(http_state_t new_state, uint32_t timeout_ms) {
  sm.current_state = new_state;
  reset_timer(timeout_ms);
}

void hal_uart_Init(void) {
  /*
   * Select Port 1
   * Set Pin 2, 3 to input Primary Module Function or PySEL1,0 = [01],
   *  (UCA1TXD/UCA1SIMO, UCA1RXD/UCA1SOMI).
   */
  GPIO_setAsPeripheralModuleFunctionInputPin(
      GPIO_PORT_P8, GPIO_PIN2 + GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

  // Configure UART
  EUSCI_A_UART_initParam param = {0};
  param.parity = EUSCI_A_UART_NO_PARITY;
  param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
  param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
  param.uartMode = EUSCI_A_UART_MODE;

#if ((HAL_SYS_SMCLK_FREQ_HZ == 8000000) && (HAL_UART_BAUDRATE == 57600))
  param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
  param.clockPrescalar = 8; // 8000000/16/57600 = 8.68
  param.firstModReg = 10;   // UCOS16=1, BRFx = 10, BRSx = 0xF7
  param.secondModReg = 0xF7;
  param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;
#elif ((HAL_SYS_SMCLK_FREQ_HZ == 8000000) && (HAL_UART_BAUDRATE == 115200))
  param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
  param.clockPrescalar = 4; // 8000000/16/115200 = 4.34
  param.firstModReg = 5;    // UCOS16=1, BRFx = 5, BRSx = 0x55
  param.secondModReg = 0x55;
  param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;
#elif ((HAL_UART_BAUDRATE == 9600) && (HAL_SYS_ACLK_FREQ_HZ == 32768))
  param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_ACLK;
  param.clockPrescalar = 3; // 32768/9600 = 3.41
  param.firstModReg = 0;    // UCOS16=0 BRFx = 0, BRSx = 0x92
  param.secondModReg = 0x92;
  param.overSampling = EUSCI_A_UART_LOW_FREQUENCY_BAUDRATE_GENERATION;
#else
#error "UART Baudrate Configuration not supported "
#endif

  if (STATUS_FAIL == EUSCI_A_UART_init(HAL_UART_BASE_ADDR, &param)) {
    return;
  }
  EUSCI_A_UART_enable(HAL_UART_BASE_ADDR);
}

void hal_uart_RxEnableIntMode(void) {
  // Enable USCI_A0 RX interrupt
  EUSCI_A_UART_clearInterrupt(HAL_UART_BASE_ADDR,
                              EUSCI_A_UART_RECEIVE_INTERRUPT);
  EUSCI_A_UART_enableInterrupt(HAL_UART_BASE_ADDR,
                               EUSCI_A_UART_RECEIVE_INTERRUPT);
}

void hal_uart_RxDisableIntMode(void) {
  // Disable USCI_A1 RX interrupt
  EUSCI_A_UART_disableInterrupt(HAL_UART_BASE_ADDR,
                                EUSCI_A_UART_RECEIVE_INTERRUPT);
}

void hal_uart_TxByte(uint8_t Data) {
  EUSCI_A_UART_transmitData(HAL_UART_BASE_ADDR, Data);
}

void _TxString(uint8_t *Data) {
  while (*Data != 0x00) {
    // Load data onto buffer
    hal_uart_TxByte(*Data++);
  }
}

void create_json(float input, char *Data, size_t max_len) {
  snprintf(Data, max_len, "{\"value\":%f}", input);
}

bool Send_NewData(float dt) {
  if (sm.current_state != STATE_IDLE)
    return false;

	dt = fabsf(dt);

  sm.data_to_send = dt;
  create_json(dt, sm.json_buffer, sizeof(sm.json_buffer));
  sm.retry_count = 0;
  change_state(STATE_AT_TEST, 1000);

  return true;
}

bool Send_SendATCommands() {
  switch (sm.current_state) {
  case STATE_IDLE:
    return true; // Sẵn sàng nhận data mới

  case STATE_AT_TEST:
    _TxString((uint8_t *)"AT\r\n");
    change_state(STATE_CHECK_SIM, 1000);
    break;

  case STATE_CHECK_SIM:
    if (is_timeout()) {
      _TxString((uint8_t *)"AT+CPIN?\r\n");
      change_state(STATE_CHECK_SIGNAL, 1000);
    }
    break;

  case STATE_CHECK_SIGNAL:
    if (is_timeout()) {
      _TxString((uint8_t *)"AT+CSQ\r\n");
      change_state(STATE_CHECK_NETWORK, 1000);
    }
    break;

  case STATE_CHECK_NETWORK:
    if (is_timeout()) {
      _TxString((uint8_t *)"AT+CREG?\r\n");
      change_state(STATE_SET_APN, 1000);
    }
    break;

  case STATE_SET_APN:
    if (is_timeout()) {
      _TxString((uint8_t *)"AT+CGDCONT=1,\"IP\",\"v-internet\"\r\n");
      change_state(STATE_ACTIVATE_PDP, 2000);
    }
    break;

  case STATE_ACTIVATE_PDP:
    if (is_timeout()) {
      _TxString((uint8_t *)"AT+CGACT=1,1\r\n");
      change_state(STATE_GET_IP, 2000);
    }
    break;

  case STATE_GET_IP:
    if (is_timeout()) {
      _TxString((uint8_t *)"AT+CGPADDR=1\r\n");
      change_state(STATE_HTTP_INIT, 1000);
    }
    break;

  case STATE_HTTP_INIT:
    if (is_timeout()) {
      _TxString((uint8_t *)"AT+HTTPINIT\r\n");
      change_state(STATE_SET_CID, 1000);
    }
    break;

  case STATE_SET_CID:
    if (is_timeout()) {
      _TxString((uint8_t *)"AT+HTTPPARA=\"CID\",1\r\n");
      change_state(STATE_SET_URL, 1000);
    }
    break;

  case STATE_SET_URL:
    if (is_timeout()) {
      _TxString(
          (uint8_t
               *)"AT+HTTPPARA=\"URL\",\"http://140.82.45.248:3000/data\"\r\n");
      change_state(STATE_SET_CONTENT_TYPE, 1000);
    }
    break;

  case STATE_SET_CONTENT_TYPE:
    if (is_timeout()) {
      _TxString((uint8_t *)"AT+HTTPPARA=\"CONTENT\",\"application/json\"\r\n");
      change_state(STATE_SET_DATA_LENGTH, 1000);
    }
    break;

  case STATE_SET_DATA_LENGTH:
    if (is_timeout()) {
      char dataCmd[32];
      snprintf(dataCmd, sizeof(dataCmd), "AT+HTTPDATA=%lu,10000\r\n",
               strlen(sm.json_buffer));
      _TxString((uint8_t *)dataCmd);
      change_state(STATE_SEND_JSON_DATA, 1000);
    }
    break;

  case STATE_SEND_JSON_DATA:
    if (is_timeout()) {
      _TxString((uint8_t *)sm.json_buffer);
      change_state(STATE_HTTP_POST, 1000);
    }
    break;

  case STATE_HTTP_POST:
    if (is_timeout()) {
      _TxString((uint8_t *)"AT+HTTPACTION=1\r\n");
      change_state(STATE_WAIT_RESPONSE, 2000); // Timeout dài hơn cho POST
    }
    break;

  case STATE_WAIT_RESPONSE:
    if (is_timeout()) {
      // Hoàn thành thành công
      _TxString((uint8_t *)"AT+HTTPTERM\r\n");
      change_state(STATE_COMPLETE, 2000);
    }
    break;

  case STATE_COMPLETE:
    if (is_timeout()) {
      // Dọn dẹp và quay về IDLE
      change_state(STATE_IDLE, 0);
      return true; // Hoàn thành
    }
    break;

  case STATE_ERROR:
    // Xử lý lỗi - có thể retry hoặc về IDLE
    if (sm.retry_count < sm.max_retries) {
      sm.retry_count++;
      change_state(STATE_AT_TEST, 1000);
    } else {
      change_state(STATE_IDLE, 0);
      return true; // Kết thúc với lỗi
    }
    break;
  }

  return false; // Chưa hoàn thành
}

http_state_t get_current_state(void) { return sm.current_state; }

// Hàm để force reset về IDLE (trong trường hợp khẩn cấp)
void reset_state_machine(void) {
  sm.current_state = STATE_IDLE;
  sm.retry_count = 0;
}

// Hàm để kiểm tra xem có thể gửi data mới không
bool is_ready_for_new_data(void) { return (sm.current_state == STATE_IDLE); }

// void Setup_4G_Viettel()
// {
//     _TxString((uint8_t *)"AT\r\n");
//     __delay_cycles(8000000);

//     _TxString((uint8_t *)"AT+CPIN?\r\n");
//     __delay_cycles(8000000);

//     _TxString((uint8_t *)"AT+CSQ\r\n");
//     __delay_cycles(8000000);

//     _TxString((uint8_t *)"AT+CREG?\r\n");
//     __delay_cycles(8000000);

//     _TxString((uint8_t *)"AT+CGDCONT=1,\"IP\",\"v-internet\"\r\n");
//     __delay_cycles(8000000);

//     _TxString((uint8_t *)"AT+CGACT=1,1\r\n");
//     __delay_cycles(8000000);

//     _TxString((uint8_t *)"AT+CGPADDR=1\r\n");
//     __delay_cycles(8000000);
// }

// void Send_SendATCommands(float dt)
// {
//     Setup_4G_Viettel();

//     char json[64];
//     create_json(dt, json, sizeof(json));

//         // Configure HTTP service
//     _TxString((uint8_t *)"AT+HTTPINIT\r\n");
//     __delay_cycles(8000000);

//     // Set HTTP parameters
//     _TxString((uint8_t *)"AT+HTTPPARA=\"CID\",1\r\n");
//     __delay_cycles(8000000);

//     // Set the URL
//     _TxString((uint8_t
//     *)"AT+HTTPPARA=\"URL\",\"http://140.82.45.248:1000/data\"\r\n");
//     __delay_cycles(8000000);

//     // Set content type to JSON
//     _TxString((uint8_t *)"AT+HTTPPARA=\"CONTENT\",\"application/json\"\r\n");
//     __delay_cycles(8000000);

//     char dataCmd[32];
//     snprintf(dataCmd, sizeof(dataCmd), "AT+HTTPDATA=%lu,10000\r\n",
//     strlen(json)); _TxString((uint8_t *)dataCmd);
//     __delay_cycles(8000000);
//        // Gửi dữ liệu JSON
//     _TxString((uint8_t *)json);
//     __delay_cycles(8000000);  // Chờ module xử lý

//         // Gửi lệnh POST
//     _TxString((uint8_t *)"AT+HTTPACTION=1\r\n");  // 1 = POST
//     __delay_cycles(2000000);

// }

bool WaitForOK(uint32_t timeout_ms) {
  char response[64];
  uint8_t idx = 0;

  uint32_t start = 0; // giả sử bạn có hàm millis() trả về ms đã chạy

  while (1) {
    int c = UART_ReadByte();
    if (c >= 0) {
      if (idx < sizeof(response) - 1) {
        response[idx++] = (char)c;
        response[idx] = 0;

        if (strstr(response, "OK") != NULL)
          return true;
        else if (strstr(response, "ERROR") != NULL)
          return false;
      }
    }
    // Kiểm tra timeout
    // if(millis() - start > timeout_ms)
    //     return false;
  }
}

#define RX_BUFFER_SIZE 128

volatile uint8_t rxBuffer[RX_BUFFER_SIZE];
volatile uint8_t rxHead = 0;
volatile uint8_t rxTail = 0;
void __attribute__((interrupt(USCI_A0_VECTOR))) USCI_A0_ISR(void) {
  switch (__even_in_range(UCA0IV, USCI_UART_UCTXCPTIFG)) {
  case USCI_UART_UCRXIFG: {
    uint8_t data = UCA0RXBUF;

    // Lưu vào vòng đệm nếu chưa đầy
    uint8_t next = (rxHead + 1) % RX_BUFFER_SIZE;
    if (next != rxTail) {
      rxBuffer[rxHead] = data;
      rxHead = next;
    }
    // else: buffer đầy, bỏ byte
    break;
  }
  default:
    break;
  }
}

bool hal_uart_RxPeekByte(uint8_t *byte) {
  if (EUSCI_A_UART_getInterruptStatus(HAL_UART_BASE_ADDR,
                                      EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)) {
    *byte = EUSCI_A_UART_receiveData(HAL_UART_BASE_ADDR);
    EUSCI_A_UART_clearInterrupt(HAL_UART_BASE_ADDR,
                                EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG);
    return true;
  }

  return false;
}

// Interrupt Service Routines
//
//*****************************************************************************
//
//! ISR for UART
//!
//! Interrupt Service Routine for UART serial communication with a PC
//! The ISR passes the data to a Callback event which parses the data
//!
//! \return none
//
// *****************************************************************************
#pragma vector = USCI_A1_VECTOR
__interrupt void hal_uart_ISR(void) {
  switch (__even_in_range(UCA1IV, USCI_UART_UCTXCPTIFG)) {
  case USCI_NONE:
    break;
  case USCI_UART_UCRXIFG:
    // Routine is executed when a byte is received via UART
    guRxData = EUSCI_A_UART_receiveData(HAL_UART_BASE_ADDR);
    // Send the received byte to TI_Serial_Comm
    // if (hal_uart_DataRx_Event(Data) == true)
    { LPM3_EXIT; }
    break;
  case USCI_UART_UCTXIFG:
    break;
  case USCI_UART_UCSTTIFG:
    break;
  case USCI_UART_UCTXCPTIFG:
    break;
  }
}

// ------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------

// #include "TI_SerialComm.h"
// #include "USSLibGUIApp/fr6047_USS_app/lcd_statemachineDC/lcd_statemachine.h"
// #include "msp430.h"
// #include <resultsDC/results.h>
// #include <stdbool.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <string.h>

// volatile uint32_t millis_counter = 0;

// #pragma vector = TIMER0_A0_VECTOR
// __interrupt void Timer_A_ISR(void)
// {
//   millis_counter++; // Mỗi lần ngắt là 1ms
// }

// void InitTimerMillis()
// {
//   // Cấu hình CCR0 để ngắt mỗi 1ms (8MHz / 1000 = 8000 chu kỳ)
//   Timer_A_clearTimerInterrupt(TIMER_A0_BASE);

//   Timer_A_clear(TIMER_A0_BASE);

//   Timer_A_initUpModeParam timerParams = {
//       .clockSource = TIMER_A_CLOCKSOURCE_SMCLK,
//       .clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1, // 8MHz / 1 = 8MHz
//       .timerPeriod = 8000 - 1,                             // 1ms = 8000 chu kỳ
//       .timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE,
//       .captureCompareInterruptEnable_CCR0_CCIE =
//           TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,
//       .timerClear = TIMER_A_SKIP_CLEAR,
//       .startTimer = true};

//   Timer_A_initUpMode(TIMER_A0_BASE, &timerParams);
// }

// uint32_t millis() { return millis_counter; }

// static volatile uint8_t guRxData;

// typedef enum
// {
//   STATE_IDLE = 0,
//   STATE_AT_TEST,
//   STATE_CHECK_SIM,
//   STATE_CHECK_SIGNAL,
//   STATE_CHECK_NETWORK,
//   STATE_SET_APN,
//   STATE_ACTIVATE_PDP,
//   STATE_GET_IP,
//   STATE_HTTP_INIT,
//   STATE_SET_CID,
//   STATE_SET_URL,
//   STATE_SET_CONTENT_TYPE,
//   STATE_SET_DATA_LENGTH,
//   STATE_SEND_JSON_DATA,
//   STATE_HTTP_POST,
//   STATE_WAIT_RESPONSE,
//   STATE_COMPLETE,
//   STATE_ERROR
// } http_state_t;

// // Cấu trúc quản lý state machine
// typedef struct
// {
//   http_state_t current_state;
//   uint32_t state_timer;
//   uint32_t timeout_ms;
//   float data_to_send;
//   char json_buffer[64];
//   uint8_t retry_count;
//   uint8_t max_retries;
// } http_state_machine_t;

// static http_state_machine_t sm = {.current_state = STATE_IDLE,
//                                   .state_timer = 0,
//                                   .timeout_ms = 1000, // 5 giây timeout mặc định
//                                   .retry_count = 0,
//                                   .max_retries = 3};

// static bool is_timeout(void)
// {
//   return (millis() - sm.state_timer) > sm.timeout_ms;
// }

// // Reset timer cho state mới
// static void reset_timer(uint32_t timeout_ms)
// {
//   sm.state_timer = millis();
//   sm.timeout_ms = timeout_ms;
// }

// // Chuyển sang state mới
// static void change_state(http_state_t new_state, uint32_t timeout_ms)
// {
//   sm.current_state = new_state;
//   reset_timer(timeout_ms);
// }

// // Enum cho trạng thái WaitForOK
// typedef enum
// {
//   WAIT_IDLE,
//   WAIT_PENDING,
//   WAIT_OK,
//   WAIT_ERROR,
//   WAIT_TIMEOUT
// } wait_state_t;

// // Structure để lưu trạng thái WaitForOK
// typedef struct
// {
//   wait_state_t state;
//   char response[64];
//   uint8_t idx;
//   uint32_t start_time;
//   uint32_t timeout_ms;
// } wait_context_t;

// static wait_context_t wait_ctx = {WAIT_IDLE, {0}, 0, 0, 0};

// void WaitForOK_Start(uint32_t timeout_ms)
// {
//   wait_ctx.state = WAIT_PENDING;
//   wait_ctx.idx = 0;
//   memset(wait_ctx.response, 0, sizeof(wait_ctx.response));
//   wait_ctx.start_time = millis();
//   wait_ctx.timeout_ms = timeout_ms;
// }

// wait_state_t WaitForOK_Check(void)
// {
//   uint8_t byte;

//   if (wait_ctx.state != WAIT_PENDING)
//   {
//     return wait_ctx.state;
//   }

//   // Đọc dữ liệu từ UART nếu có
//   if (hal_uart_RxPeekByte(&byte))
//   {
//     if (wait_ctx.idx < sizeof(wait_ctx.response) - 1)
//     {
//       wait_ctx.response[wait_ctx.idx++] = (char)byte;
//       wait_ctx.response[wait_ctx.idx] = 0;

//       if (strstr(wait_ctx.response, "OK") != NULL)
//       {
//         wait_ctx.state = WAIT_OK;
//         return WAIT_OK;
//       }
//       else if (strstr(wait_ctx.response, "ERROR") != NULL)
//       {
//         wait_ctx.state = WAIT_ERROR;
//         return WAIT_ERROR;
//       }
//     }
//   }

//   // Kiểm tra timeout
//   if (millis() - wait_ctx.start_time > wait_ctx.timeout_ms)
//   {
//     wait_ctx.state = WAIT_TIMEOUT;
//     return WAIT_TIMEOUT;
//   }

//   return WAIT_PENDING;
// }

// void hal_uart_Init(void)
// {
//   /*
//    * Select Port 1
//    * Set Pin 2, 3 to input Primary Module Function or PySEL1,0 = [01],
//    *  (UCA1TXD/UCA1SIMO, UCA1RXD/UCA1SOMI).
//    */
//   GPIO_setAsPeripheralModuleFunctionInputPin(
//       GPIO_PORT_P8, GPIO_PIN2 + GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

//   // Configure UART
//   EUSCI_A_UART_initParam param = {0};
//   param.parity = EUSCI_A_UART_NO_PARITY;
//   param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
//   param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
//   param.uartMode = EUSCI_A_UART_MODE;

// #if ((HAL_SYS_SMCLK_FREQ_HZ == 8000000) && (HAL_UART_BAUDRATE == 57600))
//   param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
//   param.clockPrescalar = 8; // 8000000/16/57600 = 8.68
//   param.firstModReg = 10;   // UCOS16=1, BRFx = 10, BRSx = 0xF7
//   param.secondModReg = 0xF7;
//   param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;
// #elif ((HAL_SYS_SMCLK_FREQ_HZ == 8000000) && (HAL_UART_BAUDRATE == 115200))
//   param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
//   param.clockPrescalar = 4; // 8000000/16/115200 = 4.34
//   param.firstModReg = 5;    // UCOS16=1, BRFx = 5, BRSx = 0x55
//   param.secondModReg = 0x55;
//   param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;
// #elif ((HAL_UART_BAUDRATE == 9600) && (HAL_SYS_ACLK_FREQ_HZ == 32768))
//   param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_ACLK;
//   param.clockPrescalar = 3; // 32768/9600 = 3.41
//   param.firstModReg = 0;    // UCOS16=0 BRFx = 0, BRSx = 0x92
//   param.secondModReg = 0x92;
//   param.overSampling = EUSCI_A_UART_LOW_FREQUENCY_BAUDRATE_GENERATION;
// #else
// #error "UART Baudrate Configuration not supported "
// #endif

//   if (STATUS_FAIL == EUSCI_A_UART_init(HAL_UART_BASE_ADDR, &param))
//   {
//     return;
//   }
//   hal_uart_RxEnableIntMode();
//   EUSCI_A_UART_enable(HAL_UART_BASE_ADDR);
// }

// void hal_uart_RxEnableIntMode(void)
// {
//   // Enable USCI_A0 RX interrupt
//   EUSCI_A_UART_clearInterrupt(HAL_UART_BASE_ADDR,
//                               EUSCI_A_UART_RECEIVE_INTERRUPT);
//   EUSCI_A_UART_enableInterrupt(HAL_UART_BASE_ADDR,
//                                EUSCI_A_UART_RECEIVE_INTERRUPT);
// }

// void hal_uart_RxDisableIntMode(void)
// {
//   // Disable USCI_A1 RX interrupt
//   EUSCI_A_UART_disableInterrupt(HAL_UART_BASE_ADDR,
//                                 EUSCI_A_UART_RECEIVE_INTERRUPT);
// }

// void hal_uart_TxByte(uint8_t Data)
// {
//   EUSCI_A_UART_transmitData(HAL_UART_BASE_ADDR, Data);
// }

// void _TxString(uint8_t *Data)
// {
//   while (*Data != 0x00)
//   {
//     // Load data onto buffer
//     hal_uart_TxByte(*Data++);
//   }
// }

// void create_json(float input, char *Data, size_t max_len)
// {
//   snprintf(Data, max_len, "{\"value\":%f}", input);
// }

// bool Send_NewData(float dt)
// {
//   if (sm.current_state != STATE_IDLE)
//     return false;

//   dt = fabsf(dt);

//   sm.data_to_send = dt;
//   create_json(dt, sm.json_buffer, sizeof(sm.json_buffer));
//   sm.retry_count = 3;
//   change_state(STATE_AT_TEST, 1000);

//   return true;
// }
// typedef enum
// {
//   CMD_SEND,
//   CMD_WAIT_RESPONSE
// } cmd_state_t;

// bool Send_SendATCommands()
// {
//   static cmd_state_t cmd_state = CMD_SEND;
//   wait_state_t wait_result;

//   switch (sm.current_state)
//   {
//   case STATE_IDLE:
//     cmd_state = CMD_SEND; // Reset cho lần chạy tiếp theo
//     return true;          // Sẵn sàng nhận data mới

//   case STATE_AT_TEST:
//     if (cmd_state == CMD_SEND)
//     {
//       _TxString((uint8_t *)"AT\r\n");
//       WaitForOK_Start(1000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_CHECK_SIM, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_CHECK_SIM:
//     if (cmd_state == CMD_SEND)
//     {
//       _TxString((uint8_t *)"AT+CPIN?\r\n");
//       WaitForOK_Start(1000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_CHECK_SIGNAL, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_CHECK_SIGNAL:
//     if (cmd_state == CMD_SEND)
//     {
//       _TxString((uint8_t *)"AT+CSQ\r\n");
//       WaitForOK_Start(1000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_CHECK_NETWORK, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_CHECK_NETWORK:
//     if (cmd_state == CMD_SEND)
//     {
//       _TxString((uint8_t *)"AT+CREG?\r\n");
//       WaitForOK_Start(1000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_SET_APN, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_SET_APN:
//     if (cmd_state == CMD_SEND)
//     {
//       _TxString((uint8_t *)"AT+CGDCONT=1,\"IP\",\"v-internet\"\r\n");
//       WaitForOK_Start(2000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_ACTIVATE_PDP, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_ACTIVATE_PDP:
//     if (cmd_state == CMD_SEND)
//     {
//       _TxString((uint8_t *)"AT+CGACT=1,1\r\n");
//       WaitForOK_Start(5000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_GET_IP, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_GET_IP:
//     if (cmd_state == CMD_SEND)
//     {
//       _TxString((uint8_t *)"AT+CGPADDR=1\r\n");
//       WaitForOK_Start(1000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_HTTP_INIT, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_HTTP_INIT:
//     if (cmd_state == CMD_SEND)
//     {
//       _TxString((uint8_t *)"AT+HTTPINIT\r\n");
//       WaitForOK_Start(1000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_SET_CID, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_SET_CID:
//     if (cmd_state == CMD_SEND)
//     {
//       _TxString((uint8_t *)"AT+HTTPPARA=\"CID\",1\r\n");
//       WaitForOK_Start(1000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_SET_URL, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_SET_URL:
//     if (cmd_state == CMD_SEND)
//     {
//       _TxString((uint8_t *)"AT+HTTPPARA=\"URL\",\"http://140.82.45.248:3000/data\"\r\n");
//       WaitForOK_Start(1000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_SET_CONTENT_TYPE, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_SET_CONTENT_TYPE:
//     if (cmd_state == CMD_SEND)
//     {
//       _TxString((uint8_t *)"AT+HTTPPARA=\"CONTENT\",\"application/json\"\r\n");
//       WaitForOK_Start(1000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_SET_DATA_LENGTH, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_SET_DATA_LENGTH:
//     if (cmd_state == CMD_SEND)
//     {
//       char dataCmd[32];
//       snprintf(dataCmd, sizeof(dataCmd), "AT+HTTPDATA=%lu,10000\r\n",
//                strlen(sm.json_buffer));
//       _TxString((uint8_t *)dataCmd);
//       WaitForOK_Start(1000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_SEND_JSON_DATA, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_SEND_JSON_DATA:
//     if (cmd_state == CMD_SEND)
//     {
//       _TxString((uint8_t *)sm.json_buffer);
//       WaitForOK_Start(1000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_HTTP_POST, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_HTTP_POST:
//     if (cmd_state == CMD_SEND)
//     {
//       _TxString((uint8_t *)"AT+HTTPACTION=1\r\n");
//       WaitForOK_Start(5000); // Timeout dài hơn cho POST request
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_WAIT_RESPONSE, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_WAIT_RESPONSE:
//     if (cmd_state == CMD_SEND)
//     {
//       // Đọc response từ server (nếu cần)
//       _TxString((uint8_t *)"AT+HTTPREAD\r\n");
//       WaitForOK_Start(1000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       if (wait_result == WAIT_OK)
//       {
//         change_state(STATE_COMPLETE, 0);
//         cmd_state = CMD_SEND;
//       }
//       else if (wait_result == WAIT_ERROR || wait_result == WAIT_TIMEOUT)
//       {
//         change_state(STATE_ERROR, 0);
//         cmd_state = CMD_SEND;
//       }
//     }
//     break;

//   case STATE_COMPLETE:
//     if (cmd_state == CMD_SEND)
//     {
//       // Terminate HTTP session
//       _TxString((uint8_t *)"AT+HTTPTERM\r\n");
//       WaitForOK_Start(1000);
//       cmd_state = CMD_WAIT_RESPONSE;
//     }
//     else
//     {
//       wait_result = WaitForOK_Check();
//       // Không quan tâm kết quả terminate
//       change_state(STATE_IDLE, 0);
//       sm.retry_count = 0; // Reset retry counter
//       cmd_state = CMD_SEND;
//       return true; // Hoàn thành thành công
//     }
//     break;

//   case STATE_ERROR:
//     // if (cmd_state == CMD_SEND) {
//     //   // Cleanup trước khi retry
//     //   _TxString((uint8_t *)"AT+HTTPTERM\r\n");
//     //   WaitForOK_Start(2000);
//     //   cmd_state = CMD_WAIT_RESPONSE;
//     // } else {
//     //   wait_result = WaitForOK_Check();
//     //   // Không quan tâm kết quả cleanup
//     //   if (wait_result != WAIT_PENDING) {
//     // Xử lý lỗi - retry tối đa 3 lần
//     if (sm.retry_count < 3)
//     {
//       sm.retry_count++;
//       change_state(STATE_AT_TEST, 0);
//     }
//     else
//     {
//       // Đã retry đủ 3 lần, quay về IDLE
//       sm.retry_count = 0;
//       change_state(STATE_IDLE, 0);
//       cmd_state = CMD_SEND;
//       return true; // Kết thúc với lỗi
//     }
//     // cmd_state = CMD_SEND;
//     // }
//     // }
//     break;
//   }

//   return false; // Chưa hoàn thành
// }

// http_state_t get_current_state(void) { return sm.current_state; }

// // Hàm để force reset về IDLE (trong trường hợp khẩn cấp)
// void reset_state_machine(void)
// {
//   sm.current_state = STATE_IDLE;
//   sm.retry_count = 0;
// }

// // Hàm để kiểm tra xem có thể gửi data mới không
// bool is_ready_for_new_data(void) { return (sm.current_state == STATE_IDLE); }

// // #define RX_BUFFER_SIZE 128

// // volatile uint8_t rxBuffer[RX_BUFFER_SIZE];
// // volatile uint8_t rxHead = 0;
// // volatile uint8_t rxTail = 0;
// // void __attribute__((interrupt(USCI_A3_VECTOR))) USCI_A3_ISR(void)
// // {
// //   switch (__even_in_range(UCA3IV, USCI_UART_UCTXCPTIFG))
// //   {
// //   case USCI_UART_UCRXIFG:
// //   {
// //     uint8_t data = UCA3RXBUF;

// //     // Lưu vào vòng đệm nếu chưa đầy
// //     uint8_t next = (rxHead + 1) % RX_BUFFER_SIZE;
// //     if (next != rxTail)
// //     {
// //       rxBuffer[rxHead] = data;
// //       rxHead = next;
// //     }
// //     // else: buffer đầy, bỏ byte
// //     break;
// //   }
// //   default:
// //     break;
// //   }
// // }

// bool hal_uart_RxPeekByte(uint8_t *byte)
// {
//     if ( EUSCI_A_UART_getInterruptStatus(HAL_UART_BASE_ADDR,
//                                         EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG) )
//     {
//         *byte = EUSCI_A_UART_receiveData(HAL_UART_BASE_ADDR);
//         EUSCI_A_UART_clearInterrupt(HAL_UART_BASE_ADDR,
//                                     EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG);
//         return true;
//     }

//     return false;
// }

// // Interrupt Service Routines
// //
// //*****************************************************************************
// //
// //! ISR for UART
// //!
// //! Interrupt Service Routine for UART serial communication with a PC
// //! The ISR passes the data to a Callback event which parses the data
// //!
// //! \return none
// //
// // *****************************************************************************
// #pragma vector=USCI_A3_VECTOR
// __interrupt void hal_uart_ISR(void)
// {
//     switch(__even_in_range(UCA3IV,USCI_UART_UCTXCPTIFG))
//     {
//     case USCI_NONE: break;
//     case USCI_UART_UCRXIFG:
//         // Routine is executed when a byte is received via UART
//         guRxData = EUSCI_A_UART_receiveData(HAL_UART_BASE_ADDR);
//         // Send the received byte to TI_Serial_Comm
//         //if (hal_uart_DataRx_Event(Data) == true)
//         {
//             LPM3_EXIT;
//         }
//       break;
//     case USCI_UART_UCTXIFG: break;
//     case USCI_UART_UCSTTIFG: break;
//     case USCI_UART_UCTXCPTIFG: break;
//   }
// }
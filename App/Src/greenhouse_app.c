#include "board_role.h"
#if BOARD_ROLE != BOARD_STANDALONE
#include "main.h"
#include "greenhouse_app.h"
#include "greenhouse_server.h"
#include "greenhouse_client.h"
#include "greenhouse_motor.h"
#include "uart_link.h"
#include "display.h"
#include "monitor_config.h"
#include "dht11.h"
#include "led_control.h"
#include "stepper_control.h"
#include <stdio.h>
#include <string.h>

static uint32_t last_screen;
static void show_number(uint8_t row, const char *label, int value, const char *unit) {
  char line[40];
  unsigned n = (unsigned)(value < 0 ? -value : value);
  snprintf(line, sizeof line, "%s: %s%u.%u %s", label, value < 0 ? "-" : "", n/10, n%10, unit);
  Display_Line(row, line);
}
static void show_u16(uint8_t row, const char *label, unsigned n) {
  char line[40]; snprintf(line, sizeof line, "%s: %u", label, n); Display_Line(row, line);
}

#if BOARD_ROLE == BOARD_SENSOR
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;
static DhtReading reading;
static DhtStatus dht_status;
static uint32_t last_dht, last_adc, last_uart;
static uint16_t adc_raw;
static int adc_valid;
static GreenhouseClient client;

static int adc_read(uint16_t *raw) {
  uint32_t sum = 0;
  for (unsigned i = 0; i < ADC_AVERAGE_SAMPLES; ++i) {
    if (HAL_ADC_Start(&hadc1) != HAL_OK) return 0;
    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) { HAL_ADC_Stop(&hadc1); return 0; }
    sum += HAL_ADC_GetValue(&hadc1);
    if (HAL_ADC_Stop(&hadc1) != HAL_OK) return 0;
  }
  *raw = (uint16_t)((sum + ADC_AVERAGE_SAMPLES/2) / ADC_AVERAGE_SAMPLES);
  return 1;
}
static void exchange(void) {
  LinkMessage request;
  uint8_t tx[LINK_PACKET_SIZE], rx[LINK_PACKET_SIZE];
  GreenhouseClient_Request(&client, &request);
  LinkProtocol_Encode(tx, &request);
  if (!UartLink_Exchange(tx, rx)) { ++client.bad_replies; return; }
  GreenhouseClient_Reply(&client, rx, HAL_GetTick());
}
void GreenhouseApp_Init(void) {
  if (HAL_TIM_Base_Start(&htim2) != HAL_OK || HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) Error_Handler();
  LedControl_Init();
  UartLink_Init();
  GreenhouseClient_Init(&client, (uint16_t)(HAL_GetTick() ^ 0xA17FU));
  Display_Init();
  Display_Line(0, "BOARD1 SENSOR UART2 V3");
  Display_Line(1, "REMOTE: WAIT UART");
  Display_Line(9, "DHT11: WARM-UP");
  /* During warm-up the receiver can already answer, without polling DHT11 early. */
  uint32_t start = HAL_GetTick();
  while ((uint32_t)(HAL_GetTick() - start) < 2200U) {
    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - last_uart) >= 200U) { last_uart = now; exchange(); }
  }
  last_dht = HAL_GetTick() - DHT_INTERVAL_MS;
  last_adc = HAL_GetTick() - ADC_UPDATE_MS;
  last_screen = HAL_GetTick() - 250U;
}
void GreenhouseApp_Run(void) {
  uint32_t now = HAL_GetTick();
  if ((uint32_t)(now - last_dht) >= DHT_INTERVAL_MS) {
    dht_status = DHT11_Read(&reading); last_dht = HAL_GetTick();
    GreenhouseClient_Measurement(&client, reading.temperature10,
                                 reading.humidity10, dht_status == DHT_OK);
  }
  now = HAL_GetTick();
  if ((uint32_t)(now - last_uart) >= 200U) { last_uart = now; exchange(); }
  now = HAL_GetTick();
  if ((uint32_t)(now - last_adc) >= ADC_UPDATE_MS) {
    last_adc = now; adc_valid = adc_read(&adc_raw);
    LedControl_Update(adc_raw, adc_valid);
  }
  if ((uint32_t)(now - last_screen) < 250U) return;
  last_screen = now;
  int online = GreenhouseClient_Online(&client, now);
  Display_Line(1, !online ? "REMOTE: NO UART REPLY" : !client.ready ? "REMOTE: SYNC" :
    (client.remote.flags & LINK_STATUS_MOVING) ?
      ((client.remote.detail & LINK_DETAIL_TARGET_OPEN) ? "REMOTE: OPENING" : "REMOTE: CLOSING") :
    (client.remote.flags & LINK_STATUS_OPEN) ? "REMOTE: OPEN" :
    (client.remote.detail & LINK_DETAIL_AT_CLOSED) ? "REMOTE: CLOSED" : "REMOTE: STOPPED");
  if (dht_status == DHT_OK) {
    show_number(2, "TEMP", reading.temperature10, "C");
    show_number(3, "HUM", reading.humidity10, "%");
  } else { Display_Line(2, "TEMP: --"); Display_Line(3, "HUM: --"); }
  Display_Line(4, DHT11_DiagnosticText());
  if (adc_valid) {
    show_u16(5, "ADC", adc_raw);
    uint32_t mv = ((uint32_t)adc_raw * ADC_VREF_MV + 2047U) / 4095U;
    char line[40]; snprintf(line, sizeof line, "U ADC: %lu.%03lu V", (unsigned long)(mv/1000), (unsigned long)(mv%1000));
    Display_Line(6, line);
  } else { Display_Line(5, "ADC: ERROR"); Display_Line(6, "U ADC: --"); }
  Display_Line(7, !online ? "UART: OFFLINE" : !client.ready ? "UART: SYNC" :
    (client.remote.flags & LINK_STATUS_STALE) ? "UART: OK / DATA STALE" :
    (client.remote.flags & LINK_STATUS_SENSOR_ERROR) ? "UART: OK / SENSOR ERROR" :
    (client.remote.flags & LINK_STATUS_RX_ERROR) ? "UART: OK / RX ERROR" : "UART: OK");
  char line[40]; snprintf(line, sizeof line, "TX:%u ACK:%u %s", client.sequence,
    client.have_reply && (client.remote.flags & LINK_STATUS_HAVE_MEASUREMENT) ? client.remote.sequence : 0,
    client.pending ? "WAIT" : "OK");
  Display_Line(8, line);
  UartLinkDiagnostics diagnostics; UartLink_GetDiagnostics(&diagnostics);
  snprintf(line, sizeof line, "RX:%02X %02X %02X %02X N:%u",
    diagnostics.last_rx[0], diagnostics.last_rx[1], diagnostics.last_rx[2],
    diagnostics.last_rx[3], diagnostics.last_size);
  Display_Line(9, line);
}
#else
static GreenhouseServer server;
static int16_t temperature10;
static uint16_t humidity10;
static uint32_t seen_errors;
static void reply_status(uint32_t now) {
  LinkMessage m; uint8_t packet[LINK_PACKET_SIZE];
  GreenhouseServer_Status(&server, now, StepperControl_GetPositionSteps(),
                          StepperControl_IsMoving(), &m);
  LinkProtocol_Encode(packet, &m);
  (void)UartLink_Send(packet);
}
void GreenhouseApp_Init(void) {
  GreenhouseServer_Init(&server);
  GreenhouseMotor_Init();
  Display_Init();
  Display_Line(0, "BOARD2 STEP UART2 V3");
  Display_Line(1, "STEPPER: CLOSED ZERO");
  last_screen = HAL_GetTick() - 250U;
  UartLink_Init();
}
void GreenhouseApp_Run(void) {
  uint32_t now = HAL_GetTick();
  uint32_t errors = UartLink_ErrorCount();
  if (errors != seen_errors) { server.rx_error = 1; seen_errors = errors; }
  uint8_t packet[LINK_PACKET_SIZE]; LinkMessage message;
  while (UartLink_Receive(packet)) {
    now = HAL_GetTick();
    if (!LinkProtocol_Decode(&message, packet) ||
        (message.type != LINK_TYPE_HELLO && message.type != LINK_TYPE_POLL &&
         message.type != LINK_TYPE_MEASUREMENT)) {
      server.rx_error = 1; continue;
    }
    if (GreenhouseServer_Receive(&server, &message, now)) {
      if (message.flags & LINK_DATA_VALID) {
        temperature10 = message.temperature10; humidity10 = message.humidity10;
      }
    }
    GreenhouseMotor_Update(&server, now);
    reply_status(now);
  }
  now = HAL_GetTick(); Greenhouse_Tick(&server.control, now);
  GreenhouseMotor_Update(&server, now);
  if ((uint32_t)(now - last_screen) < 250U) return;
  last_screen = now;
  uint16_t position = StepperControl_GetPositionSteps();
  Display_Line(1, StepperControl_IsMoving() ? (server.control.open ? "STEPPER: OPENING" : "STEPPER: CLOSING") :
    position == GH_STEPPER_OPEN_STEPS ? "STEPPER: OPEN" :
    position == GH_STEPPER_CLOSED_STEPS ? "STEPPER: CLOSED" : "STEPPER: STOPPED");
  int stale = Greenhouse_Stale(&server.control, now);
  if (server.measurement_valid && !stale) {
    show_number(2, "TEMP", temperature10, "C"); show_number(3, "HUM", humidity10, "%");
  } else { Display_Line(2, "TEMP: --"); Display_Line(3, "HUM: --"); }
  Display_Line(4, stale ? "DATA: STALE / HOLD" : server.control.sensor_error ? "DHT ERROR / HOLD" : "DATA: VALID");
  UartLinkDiagnostics diagnostics; UartLink_GetDiagnostics(&diagnostics);
  if (server.session_ready) show_u16(5, "ACK SEQUENCE", server.have_measurement ? server.sequence : 0);
  else {
    char header[40]; snprintf(header, sizeof header, "IN:%02X %02X %02X %02X",
      diagnostics.last_rx[0], diagnostics.last_rx[1], diagnostics.last_rx[2], diagnostics.last_rx[3]);
    Display_Line(5, header);
  }
  char line[40]; snprintf(line, sizeof line, "STEP:%u TARGET:%u", position, StepperControl_GetTargetSteps());
  Display_Line(6, line);
  Display_Line(7, server.rx_error ? "UART: BAD FRAME / CRC" : !server.session_ready ? "UART: WAIT HELLO" : "UART: SESSION READY");
  uint8_t reason = Greenhouse_Reasons(&server.control);
  Display_Line(8, reason == 3 ? "REQUEST: HUM + TEMP" : reason == 1 ? "REQUEST: HUMIDITY" : reason == 2 ? "REQUEST: TEMPERATURE" : "REQUEST: NONE");
  snprintf(line, sizeof line, "B:%lu R:%lu E:%lu",
    (unsigned long)(diagnostics.rx_bytes % 100000U),
    (unsigned long)(diagnostics.rx_frames % 100000U),
    (unsigned long)(diagnostics.errors % 100000U));
  Display_Line(9, line);
}
#endif
#endif

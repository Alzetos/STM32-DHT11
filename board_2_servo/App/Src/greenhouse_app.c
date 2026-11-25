#include "board_role.h"
#if BOARD_ROLE != BOARD_STANDALONE
#include "main.h"
#include "greenhouse_app.h"
#include "greenhouse_server.h"
#include "greenhouse_client.h"
#include "spi_link.h"
#include "display.h"
#include "monitor_config.h"
#include "servo_control.h"
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
static uint32_t last_dht, last_adc, last_spi;
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
static void exchange(uint32_t now) {
  SpiMessage request;
  uint8_t tx[SPI_PACKET_SIZE], rx[SPI_PACKET_SIZE];
  GreenhouseClient_Request(&client, &request);
  SpiProtocol_Encode(tx, &request);
  if (!SpiLink_MasterExchange(tx, rx)) { ++client.bad_replies; return; }
  GreenhouseClient_Reply(&client, rx, now);
}
void GreenhouseApp_Init(void) {
  if (HAL_TIM_Base_Start(&htim2) != HAL_OK || HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) Error_Handler();
  StepperControl_Init(); LedControl_Init();
  SpiLink_Init(0);
  GreenhouseClient_Init(&client, (uint16_t)(DWT->CYCCNT ^ HAL_GetTick()));
  Display_Init();
  Display_Line(0, "BOARD 1: SENSOR MASTER");
  Display_Line(1, "REMOTE: WAIT SPI");
  Display_Line(9, "DHT11: WARM-UP");
  /* During warm-up the receiver can already answer, without polling DHT11 early. */
  uint32_t start = HAL_GetTick();
  while ((uint32_t)(HAL_GetTick() - start) < 2200U) {
    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - last_spi) >= 100U) { last_spi = now; exchange(now); }
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
  if ((uint32_t)(now - last_spi) >= 100U) { last_spi = now; exchange(now); }
  now = HAL_GetTick();
  if ((uint32_t)(now - last_adc) >= ADC_UPDATE_MS) {
    last_adc = now; adc_valid = adc_read(&adc_raw);
    LedControl_Update(adc_raw, adc_valid); StepperControl_SetTargetFromAdc(adc_raw, adc_valid);
  }
  if ((uint32_t)(now - last_screen) < 250U) return;
  last_screen = now;
  int online = GreenhouseClient_Online(&client, now);
  Display_Line(1, !online ? "REMOTE: NO SPI REPLY" : !client.ready ? "REMOTE: SYNC" :
    (client.remote.flags & SPI_STATUS_OPEN) ? "REMOTE: OPEN" : "REMOTE: CLOSED");
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
  Display_Line(7, !online ? "SPI: OFFLINE" : !client.ready ? "SPI: SYNCHRONIZING" :
    (client.remote.flags & SPI_STATUS_STALE) ? "SPI: OK / DATA STALE" :
    (client.remote.flags & SPI_STATUS_SENSOR_ERROR) ? "SPI: OK / SENSOR ERROR" :
    (client.remote.flags & SPI_STATUS_RX_ERROR) ? "SPI: OK / RX ERROR" : "SPI: OK");
  char line[40]; snprintf(line, sizeof line, "TX:%u ACK:%u %s", client.sequence,
    client.have_reply && (client.remote.flags & SPI_STATUS_HAVE_MEASUREMENT) ? client.remote.sequence : 0,
    client.pending ? "WAIT" : "OK"); Display_Line(8, line);
  Display_Line(9, dht_status == DHT_OK ? "DHT11: OK" : "DHT11: ERROR");
}
#else
static GreenhouseServer server;
static int16_t temperature10;
static uint16_t humidity10;
static uint32_t seen_errors;
static uint8_t previous_status[SPI_PACKET_SIZE];
static int status_valid;
static void publish(uint32_t now, int initial) {
  SpiMessage m; uint8_t packet[SPI_PACKET_SIZE];
  GreenhouseServer_Status(&server, now, ServoControl_GetPulseUs(), &m);
  SpiProtocol_Encode(packet, &m);
  if (initial) SpiLink_Init(packet);
  else if (!status_valid || memcmp(previous_status, packet, sizeof packet)) SpiLink_SlavePublish(packet);
  memcpy(previous_status, packet, sizeof packet); status_valid = 1;
}
void GreenhouseApp_Init(void) {
  GreenhouseServer_Init(&server);
  ServoControl_Init();
  Display_Init();
  Display_Line(0, "BOARD 2: SERVO SLAVE");
  Display_Line(1, "SERVO CMD: CLOSED");
  last_screen = HAL_GetTick() - 250U;
  publish(HAL_GetTick(), 1);
}
void GreenhouseApp_Run(void) {
  uint32_t now = HAL_GetTick();
  uint32_t errors = SpiLink_ErrorCount();
  if (errors != seen_errors) { server.rx_error = 1; seen_errors = errors; }
  uint8_t packet[SPI_PACKET_SIZE]; SpiMessage message;
  while (SpiLink_SlaveReceive(packet)) {
    now = HAL_GetTick();
    if (!SpiProtocol_Decode(&message, packet) || message.type == SPI_TYPE_STATUS) server.rx_error = 1;
    else if (GreenhouseServer_Receive(&server, &message, now)) {
      if (message.flags & SPI_DATA_VALID) {
        temperature10 = message.temperature10; humidity10 = message.humidity10;
      }
    }
  }
  now = HAL_GetTick(); Greenhouse_Tick(&server.control, now);
  ServoControl_SetPosition(server.control.open ? SERVO_OPEN : SERVO_CLOSED);
  publish(now, 0);
  if ((uint32_t)(now - last_screen) < 250U) return;
  last_screen = now;
  Display_Line(1, server.control.open ? "SERVO CMD: OPEN" : "SERVO CMD: CLOSED");
  int stale = Greenhouse_Stale(&server.control, now);
  if (server.measurement_valid && !stale) {
    show_number(2, "TEMP", temperature10, "C"); show_number(3, "HUM", humidity10, "%");
  } else { Display_Line(2, "TEMP: --"); Display_Line(3, "HUM: --"); }
  Display_Line(4, stale ? "DATA: STALE / HOLD" : server.control.sensor_error ? "DHT ERROR / HOLD" : "DATA: VALID");
  show_u16(5, "ACK SEQUENCE", server.have_measurement ? server.sequence : 0);
  show_u16(6, "SERVO PULSE US", ServoControl_GetPulseUs());
  Display_Line(7, !server.session_ready ? "SPI: WAIT MASTER HELLO" : server.rx_error ? "SPI: RX ERROR" : "SPI: SESSION READY");
  uint8_t reason = Greenhouse_Reasons(&server.control);
  Display_Line(8, reason == 3 ? "REQUEST: HUM + TEMP" : reason == 1 ? "REQUEST: HUMIDITY" : reason == 2 ? "REQUEST: TEMPERATURE" : "REQUEST: NONE");
  show_u16(9, "CONFIRM SAMPLES", server.control.confirmations);
}
#endif
#endif

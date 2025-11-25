#include "set_functions.h"
#include "uart_handler.h"
#include "adc.h"
#include "variables.h"
#include "flash_storage.h"
#include "default_values.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char uart_buffer[512];
char uart_rx_buffer[512];
volatile bool uart_data_ready = false;
volatile uint16_t uart_rx_length = 0;
volatile bool uart_receiving = false;

void send_telemetry(void)
{
    int len = snprintf(uart_buffer, sizeof(uart_buffer),
                       "{\"status\":{"
                       "\"fwd\":%lu.%03lu,"
                       "\"ref\":%lu.%03lu,"
                       "\"trxfwd\":%lu.%03lu,"
                       "\"swr\":%lu.%03lu,"
                       "\"current\":%lu.%03lu,"
                       "\"voltage\":%lu.%03lu,"
                       "\"water_temp\":%lu.%03lu,"
                       "\"plate_temp\":%lu.%03lu,"
                       "\"alarm\":%s,"
                       "\"alert_reason\":\"%s\","
                       "\"state\":%s,"
                       "\"ptt\":%s,"
                       "\"band\":\"%s\","
                       "\"pwm_pump\":%d,"
                       "\"pwm_cooler\":%d,"
                       "\"auto_pwm_pump\":%s,"
                       "\"auto_pwm_fan\":%s,"
					   "\"protection_enabled\":%s"
                       "}}\r\n",
                       (uint32_t)forward_power,
                       (uint32_t)((forward_power - (uint32_t)forward_power) * 1000),
                       (uint32_t)reverse_power,
                       (uint32_t)((reverse_power - (uint32_t)reverse_power) * 1000),
                       (uint32_t)input_power,
                       (uint32_t)((input_power - (uint32_t)input_power) * 1000),
                       (uint32_t)swr,
                       (uint32_t)((swr - (uint32_t)swr) * 1000),
                       (uint32_t)current,
                       (uint32_t)((current - (uint32_t)current) * 1000),
                       (uint32_t)voltage,
                       (uint32_t)((voltage - (uint32_t)voltage) * 1000),
                       (uint32_t)water_temp,
                       (uint32_t)((water_temp - (uint32_t)water_temp) * 1000),
                       (uint32_t)plate_temp,
                       (uint32_t)((plate_temp - (uint32_t)plate_temp) * 1000),
                       alarm ? "true" : "false",
                       alert_reason,
                       enabled ? "true" : "false",
                       ptt ? "true" : "false",
                       current_band,
                       pwm_pump,
                       pwm_cooler,
                       auto_pwm_pump ? "true" : "false",
                       auto_pwm_fan ? "true" : "false",
                       protection_enabled ? "true" : "false");

    if (len > 0 && len < (int)sizeof(uart_buffer)) {
        HAL_UART_Transmit(&huart3, (uint8_t*)uart_buffer, len, HAL_MAX_DELAY);
    }
}

static bool parse_bool(const char *str) {
    return (strstr(str, "true") != NULL);
}

static uint8_t parse_uint8(const char *str) {
    return (uint8_t)atoi(str);
}

static bool extract_json_value(const char *json, const char *key, char *value, int value_size) {
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);

    const char *key_pos = strstr(json, search_key);
    if (key_pos == NULL) return false;

    const char *value_start = key_pos + strlen(search_key);

    // skipping spaces
    while (*value_start == ' ') value_start++;

    if (*value_start == '"') {
        // string
        value_start++; // skipping opening {
        const char *value_end = strchr(value_start, '"');
        if (value_end == NULL) return false;

        int len = value_end - value_start;
        if (len >= value_size) len = value_size - 1;

        strncpy(value, value_start, len);
        value[len] = '\0';
    } else {
        // bool or int
        const char *value_end = value_start;
        while (*value_end && *value_end != ',' && *value_end != '}' && *value_end != ' ' && *value_end != '\r' && *value_end != '\n') {
            value_end++;
        }

        int len = value_end - value_start;
        if (len >= value_size) len = value_size - 1;

        strncpy(value, value_start, len);
        value[len] = '\0';
    }

    return true;
}

// limits
static uint8_t clamp_uint8(uint8_t value, uint8_t min_val, uint8_t max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static void process_settings(const char *json) {
    char value[64];

    if (extract_json_value(json, "max_swr", value, sizeof(value))) {
        max_swr = clamp_uint8(parse_uint8(value), MAX_SWR_MIN, MAX_SWR_MAX);
    }

    if (extract_json_value(json, "max_current", value, sizeof(value))) {
        max_current = clamp_uint8(parse_uint8(value), MAX_CURRENT_MIN, MAX_CURRENT_MAX);
    }

    if (extract_json_value(json, "max_voltage", value, sizeof(value))) {
        max_voltage = clamp_uint8(parse_uint8(value), MAX_VOLTAGE_MIN, MAX_VOLTAGE_MAX);
    }

    if (extract_json_value(json, "max_water_temp", value, sizeof(value))) {
        max_water_temp = clamp_uint8(parse_uint8(value), MAX_WATER_TEMP_MIN, MAX_WATER_TEMP_MAX);
    }

    if (extract_json_value(json, "max_plate_temp", value, sizeof(value))) {
        max_plate_temp = clamp_uint8(parse_uint8(value), MAX_PLATE_TEMP_MIN, MAX_PLATE_TEMP_MAX);
    }

    if (extract_json_value(json, "max_pump_speed_temp", value, sizeof(value))) {
        max_pump_speed_temp = clamp_uint8(parse_uint8(value), MAX_PUMP_SPEED_TEMP_MIN, MAX_PUMP_SPEED_TEMP_MAX);
    }

    if (extract_json_value(json, "min_pump_speed_temp", value, sizeof(value))) {
        min_pump_speed_temp = clamp_uint8(parse_uint8(value), MIN_PUMP_SPEED_TEMP_MIN, MIN_PUMP_SPEED_TEMP_MAX);
    }

    if (extract_json_value(json, "max_fan_speed_temp", value, sizeof(value))) {
        max_fan_speed_temp = clamp_uint8(parse_uint8(value), MAX_FAN_SPEED_TEMP_MIN, MAX_FAN_SPEED_TEMP_MAX);
    }

    if (extract_json_value(json, "min_fan_speed_temp", value, sizeof(value))) {
        min_fan_speed_temp = clamp_uint8(parse_uint8(value), MIN_FAN_SPEED_TEMP_MIN, MIN_FAN_SPEED_TEMP_MAX);
    }

    if (extract_json_value(json, "max_input_power", value, sizeof(value))) {
    	max_input_power = clamp_uint8(parse_uint8(value), MIN_MAX_INPUT_POWER, MAX_MAX_INPUT_POWER);
    }

    if (extract_json_value(json, "autoband", value, sizeof(value))) {
        autoband = parse_bool(value);
    }

    if (extract_json_value(json, "default_band", value, sizeof(value))) {
        size_t len = strlen(value);
        if (len > BAND_MAX_LENGTH) {
            len = BAND_MAX_LENGTH;
        }
        strncpy(default_band, value, len);
        if (len < BAND_MAX_LENGTH) {
            memset(default_band + len, 0, BAND_MAX_LENGTH - len);
        }

        //default_band[sizeof(default_band) - 1] = '\0';
    }

    Flash_SaveAll();

    snprintf(uart_buffer, sizeof(uart_buffer), "{\"response\":\"settings updated\"}\r\n");
    HAL_UART_Transmit(&huart3, (uint8_t*)uart_buffer, strlen(uart_buffer), HAL_MAX_DELAY);
}

static void process_state(const char *json) {
    char value[64];

    if (extract_json_value(json, "ptt", value, sizeof(value))) {
        force_ptt = parse_bool(value);
    }

    if (extract_json_value(json, "pwm_pump", value, sizeof(value))) {
        pwm_pump = clamp_uint8(parse_uint8(value), PWM_PUMP_MIN, PWM_PUMP_MAX);
    }

    if (extract_json_value(json, "pwm_cooler", value, sizeof(value))) {
        pwm_cooler = clamp_uint8(parse_uint8(value), PWM_COOLER_MIN, PWM_COOLER_MAX);
    }

    if (extract_json_value(json, "band", value, sizeof(value))) {
        size_t len = strlen(value);
        if (len > BAND_MAX_LENGTH) {
            len = BAND_MAX_LENGTH;
        }
        memcpy(current_band, value, len);
        if (len < BAND_MAX_LENGTH) {
            memset(current_band + len, 0, BAND_MAX_LENGTH - len);
        }
        set_band_gpio(current_band);
    }

    if (extract_json_value(json, "auto_pwm_pump", value, sizeof(value))) {
        auto_pwm_pump = parse_bool(value);
    }

    if (extract_json_value(json, "auto_pwm_fan", value, sizeof(value))) {
        auto_pwm_fan = parse_bool(value);
    }

    if (extract_json_value(json, "enabled", value, sizeof(value))) {
        enabled = parse_bool(value);

    }

    if (extract_json_value(json, "protection_enabled", value, sizeof(value))) {
    	protection_enabled = parse_bool(value);
    }

    if (extract_json_value(json, "alarm", value, sizeof(value))) {
    	alarm = parse_bool(value);
    }

    snprintf(uart_buffer, sizeof(uart_buffer), "{\"response\":\"state updated\"}\r\n");
    HAL_UART_Transmit(&huart3, (uint8_t*)uart_buffer, strlen(uart_buffer), HAL_MAX_DELAY);
}

static void process_command(const char *json) {
    char value[64];

    if (extract_json_value(json, "value", value, sizeof(value))) {
        if (strcmp(value, "settings") == 0) {
              snprintf(uart_buffer, sizeof(uart_buffer),
                       "{\"settings\":{"
                       "\"max_swr\":%d,"
                       "\"max_current\":%d,"
                       "\"max_voltage\":%d,"
                       "\"max_water_temp\":%d,"
                       "\"max_plate_temp\":%d,"
                       "\"max_pump_speed_temp\":%d,"
                       "\"min_pump_speed_temp\":%d,"
                       "\"max_fan_speed_temp\":%d,"
                       "\"min_fan_speed_temp\":%d,"
            		   "\"max_input_power\":%d,"
                       "\"autoband\":%s,"
                       "\"default_band\":\"%s\""
                       "}}\r\n",
					   max_swr,
                       max_current,
                       max_voltage,
                       max_water_temp,
                       max_plate_temp,
                       max_pump_speed_temp,
                       min_pump_speed_temp,
                       max_fan_speed_temp,
                       min_fan_speed_temp,
					   max_input_power,
                       autoband ? "true" : "false",
                       default_band);
              HAL_UART_Transmit(&huart3, (uint8_t*)uart_buffer, strlen(uart_buffer), HAL_MAX_DELAY);
          }
        else if (strcmp(value, "status") == 0) {
            // status was here
        }
        else {
            snprintf(uart_buffer, sizeof(uart_buffer), "{\"error\":\"unknown command: %s\"}\r\n", value);
            HAL_UART_Transmit(&huart3, (uint8_t*)uart_buffer, strlen(uart_buffer), HAL_MAX_DELAY);
        }
    } else {
        snprintf(uart_buffer, sizeof(uart_buffer), "{\"error\":\"command value not found\"}\r\n");
        HAL_UART_Transmit(&huart3, (uint8_t*)uart_buffer, strlen(uart_buffer), HAL_MAX_DELAY);
    }
}

void process_received_data(void)
{
    if (uart_rx_length < sizeof(uart_rx_buffer)) {
        uart_rx_buffer[uart_rx_length] = '\0';
    } else {
        uart_rx_buffer[sizeof(uart_rx_buffer) - 1] = '\0';
    }

    if (strstr(uart_rx_buffer, "{\"command\"") == uart_rx_buffer) {
        process_command(uart_rx_buffer);
    }
    else if (strstr(uart_rx_buffer, "{\"settings\"") == uart_rx_buffer) {
        process_settings(uart_rx_buffer);
    }
    else if (strstr(uart_rx_buffer, "{\"state\"") == uart_rx_buffer) {
        process_state(uart_rx_buffer);
    }
    else {
        snprintf(uart_buffer, sizeof(uart_buffer), "{\"error\":\"unknown packet type\"}\r\n");
        HAL_UART_Transmit(&huart3, (uint8_t*)uart_buffer, strlen(uart_buffer), HAL_MAX_DELAY);
    }

    // restart receive
    uart_receive_start();
}

void uart_receive_init(void)
{
    uart_data_ready = false;
    uart_receiving = false;
    uart_rx_length = 0;
    memset(uart_rx_buffer, 0, sizeof(uart_rx_buffer));
}

void uart_receive_start(void)
{
    if (!uart_receiving) {
        uart_data_ready = false;
        uart_rx_length = 0;

        // Start receive
        if (HAL_UART_Receive_DMA(&huart3, (uint8_t*)uart_rx_buffer, sizeof(uart_rx_buffer)) == HAL_OK) {
            uart_receiving = true;

            // enable IDLE detection
            __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
        }
    }
}

void uart_receive_stop(void)
{
    if (uart_receiving) {
        // disable IDLE detection
        __HAL_UART_DISABLE_IT(&huart3, UART_IT_IDLE);
        HAL_UART_DMAStop(&huart3);
        uart_receiving = false;
    }
}

static uint16_t get_dma_received_count(void)
{
    if (huart3.hdmarx != NULL) {
        return sizeof(uart_rx_buffer) - __HAL_DMA_GET_COUNTER(huart3.hdmarx);
    }
    return 0;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        // buffer overflow
        uart_rx_length = sizeof(uart_rx_buffer);
        uart_data_ready = true;
        uart_receiving = false;
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        uart_receive_stop();
        uart_receive_start();
    }
}

// IDLE HAndler
void uart_idle_handler(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3 && uart_receiving) {
        uart_rx_length = get_dma_received_count();

        if (uart_rx_length > 0) {
            uart_data_ready = true;
            uart_receiving = false;

            // Stop DMA
            HAL_UART_DMAStop(&huart3);
        }
    }
}

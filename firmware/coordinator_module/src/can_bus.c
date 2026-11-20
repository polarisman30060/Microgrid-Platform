#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "can_bus.h"

#define CAN_TX_GPIO GPIO_NUM_20
#define CAN_RX_GPIO GPIO_NUM_19
#define CAN_BITRATE 500000
#define CAN_RX_QUEUE_LENGTH 16
#define CAN_SERVICE_TASK_STACK_SIZE 6144
#define CAN_SERVICE_TASK_PRIORITY 8
#define CAN_HEARTBEAT_PERIOD_US 100000ULL
#define CAN_HEARTBEAT_TIMEOUT_US 1000000ULL
#define CAN_DISCOVERY_TIMEOUT_US 3000000ULL
#define CAN_FAST_TELEMETRY_TIMEOUT_US 300000ULL
#define CAN_SLOW_TELEMETRY_TIMEOUT_US 3000000ULL

#define CAN_ID_COORD_HEARTBEAT 0x100
#define CAN_ID_BATTERY_FAST 0x200
#define CAN_ID_BATTERY_SLOW 0x210
#define CAN_ID_BATTERY_HEARTBEAT 0x601
#define CAN_ID_BATTERY_IDENTITY 0x701

#define CAN_NODE_TYPE_BATTERY 0x01
#define CAN_BATTERY_INSTANCE 0x00

static const char *TAG = "can_bus";
static portMUX_TYPE s_status_mux = portMUX_INITIALIZER_UNLOCKED;
static QueueHandle_t s_rx_queue;
static app_can_status_t s_status;
static twai_node_handle_t s_node;
static uint64_t s_start_time_us;
static uint64_t s_last_tx_error_log_us;
static bool s_bus_recovery_started;
static uint8_t s_heartbeat_counter;

typedef struct {
    uint32_t identifier;
    uint8_t dlc;
    uint8_t data_length;
    uint8_t data[APP_CAN_MAX_DATA_LENGTH];
    bool extended;
    bool remote;
} can_rx_frame_t;

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static int16_t read_i16_le(const uint8_t *data)
{
    uint16_t value = read_u16_le(data);
    int32_t signed_value = value < 0x8000U ? value : (int32_t)value - 0x10000;
    return (int16_t)signed_value;
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static int8_t read_i8(const uint8_t value)
{
    int16_t signed_value = value < 0x80U ? value : (int16_t)value - 0x100;
    return (int8_t)signed_value;
}

static void record_invalid_frame(bool unexpected)
{
    portENTER_CRITICAL(&s_status_mux);
    if (unexpected) {
        ++s_status.unexpected_frames;
    } else {
        ++s_status.invalid_frames;
    }
    portEXIT_CRITICAL(&s_status_mux);
}

static bool validate_frame(const can_rx_frame_t *frame)
{
    if (frame->extended || frame->remote || frame->dlc != APP_CAN_MAX_DATA_LENGTH) {
        record_invalid_frame(false);
        return false;
    }

    switch (frame->identifier) {
    case CAN_ID_BATTERY_HEARTBEAT:
    case CAN_ID_BATTERY_IDENTITY:
    case CAN_ID_BATTERY_FAST:
    case CAN_ID_BATTERY_SLOW:
        return true;
    default:
        record_invalid_frame(true);
        return false;
    }
}

static void process_battery_frame(const can_rx_frame_t *frame, uint64_t received_us)
{
    if (!validate_frame(frame)) {
        return;
    }

    portENTER_CRITICAL(&s_status_mux);
    switch (frame->identifier) {
    case CAN_ID_BATTERY_HEARTBEAT:
        if (frame->data[1] != CAN_NODE_TYPE_BATTERY || frame->data[2] > 6) {
            ++s_status.invalid_frames;
            break;
        }
        s_status.battery_local_state = frame->data[2];
        s_status.battery_warning_flags = frame->data[3];
        s_status.battery_aux_voltage_mv = read_u16_le(&frame->data[4]);
        s_status.battery_firmware_build_id = read_u16_le(&frame->data[6]);
        s_status.battery_heartbeat_us = received_us;
        break;

    case CAN_ID_BATTERY_IDENTITY:
        if (frame->data[4] != CAN_NODE_TYPE_BATTERY ||
            frame->data[5] != CAN_BATTERY_INSTANCE) {
            ++s_status.invalid_frames;
            break;
        }
        s_status.battery_serial_number = read_u32_le(frame->data);
        s_status.battery_identity_valid = true;
        s_status.battery_identity_us = received_us;
        break;

    case CAN_ID_BATTERY_FAST:
        if (frame->data[6] > 200) {
            ++s_status.invalid_frames;
            break;
        }
        s_status.battery_bus_voltage_v = (float)read_u16_le(&frame->data[0]) * 0.05f;
        s_status.battery_pack_voltage_v = (float)read_u16_le(&frame->data[2]) * 0.05f;
        s_status.battery_current_a = (float)read_i16_le(&frame->data[4]) * 0.1f;
        s_status.battery_soc_percent = (float)frame->data[6] * 0.5f;
        s_status.battery_mode_and_contactors = frame->data[7];
        s_status.battery_fast_telemetry_valid = true;
        s_status.battery_fast_telemetry_us = received_us;
        break;

    case CAN_ID_BATTERY_SLOW: {
        int8_t max_temperature_c = read_i8(frame->data[4]);
        int8_t converter_temperature_c = read_i8(frame->data[5]);
        if (max_temperature_c < -40 || max_temperature_c > 125 ||
            converter_temperature_c < -40 || converter_temperature_c > 125) {
            ++s_status.invalid_frames;
            break;
        }
        s_status.battery_min_cell_voltage_mv = read_u16_le(&frame->data[0]);
        s_status.battery_max_cell_voltage_mv = read_u16_le(&frame->data[2]);
        s_status.battery_max_temperature_c = max_temperature_c;
        s_status.battery_converter_temperature_c = converter_temperature_c;
        s_status.battery_available_capacity_ah = (float)read_u16_le(&frame->data[6]) * 0.1f;
        s_status.battery_slow_telemetry_valid = true;
        s_status.battery_slow_telemetry_us = received_us;
        break;
    }
    }
    portEXIT_CRITICAL(&s_status_mux);
}

static bool can_rx_done_callback(
    twai_node_handle_t node,
    const twai_rx_done_event_data_t *event_data,
    void *user_context)
{
    (void)event_data;
    QueueHandle_t queue = user_context;
    uint8_t receive_buffer[APP_CAN_MAX_DATA_LENGTH] = {0};
    twai_frame_t received_frame = {
        .buffer = receive_buffer,
        .buffer_len = sizeof(receive_buffer),
    };
    if (twai_node_receive_from_isr(node, &received_frame) != ESP_OK) {
        return false;
    }

    can_rx_frame_t frame = {
        .identifier = received_frame.header.id,
        .dlc = (uint8_t)received_frame.header.dlc,
        .data_length = received_frame.header.dlc > APP_CAN_MAX_DATA_LENGTH
                           ? APP_CAN_MAX_DATA_LENGTH
                           : (uint8_t)received_frame.header.dlc,
        .extended = received_frame.header.ide,
        .remote = received_frame.header.rtr,
    };
    if (!frame.remote) {
        memcpy(frame.data, receive_buffer, frame.data_length);
    }

    BaseType_t higher_priority_task_woken = pdFALSE;
    if (xQueueSendFromISR(queue, &frame, &higher_priority_task_woken) != pdTRUE) {
        portENTER_CRITICAL_ISR(&s_status_mux);
        ++s_status.dropped_frames;
        portEXIT_CRITICAL_ISR(&s_status_mux);
    }
    return higher_priority_task_woken == pdTRUE;
}

static bool can_tx_done_callback(
    twai_node_handle_t node,
    const twai_tx_done_event_data_t *event_data,
    void *user_context)
{
    (void)node;
    (void)user_context;
    if (event_data == NULL) {
        return false;
    }

    portENTER_CRITICAL_ISR(&s_status_mux);
    if (event_data->is_tx_success) {
        ++s_status.transmitted_frames;
    } else {
        ++s_status.tx_failures;
    }
    portEXIT_CRITICAL_ISR(&s_status_mux);
    return false;
}

static bool can_state_change_callback(
    twai_node_handle_t node,
    const twai_state_change_event_data_t *event_data,
    void *user_context)
{
    (void)node;
    (void)user_context;
    if (event_data == NULL) {
        return false;
    }

    portENTER_CRITICAL_ISR(&s_status_mux);
    s_status.bus_off = event_data->new_sta == TWAI_ERROR_BUS_OFF;
    portEXIT_CRITICAL_ISR(&s_status_mux);
    return false;
}

static void update_coordinator_state(uint64_t now_us)
{
    portENTER_CRITICAL(&s_status_mux);
    s_status.battery_online =
        s_status.battery_heartbeat_us != 0 &&
        now_us - s_status.battery_heartbeat_us <= CAN_HEARTBEAT_TIMEOUT_US;
    s_status.battery_fast_telemetry_stale =
        !s_status.battery_fast_telemetry_valid ||
        now_us - s_status.battery_fast_telemetry_us > CAN_FAST_TELEMETRY_TIMEOUT_US;
    s_status.battery_slow_telemetry_stale =
        !s_status.battery_slow_telemetry_valid ||
        now_us - s_status.battery_slow_telemetry_us > CAN_SLOW_TELEMETRY_TIMEOUT_US;

    bool battery_ready =
        s_status.battery_identity_valid &&
        s_status.battery_online &&
        s_status.battery_local_state == 2;
    if (battery_ready) {
        if (s_status.coordinator_state == APP_COORD_STATE_DISCOVERY ||
            s_status.coordinator_state == APP_COORD_STATE_DEGRADED) {
            s_status.coordinator_state = APP_COORD_STATE_READY;
        }
    } else if (s_status.coordinator_state == APP_COORD_STATE_READY &&
               !s_status.battery_online) {
        s_status.coordinator_state = APP_COORD_STATE_DEGRADED;
    } else if (s_status.coordinator_state == APP_COORD_STATE_DISCOVERY &&
               now_us - s_start_time_us >= CAN_DISCOVERY_TIMEOUT_US) {
        s_status.coordinator_state = APP_COORD_STATE_FAULT;
    }
    portEXIT_CRITICAL(&s_status_mux);
}

static void transmit_coordinator_heartbeat(uint64_t now_us)
{
    uint8_t data[APP_CAN_MAX_DATA_LENGTH] = {0};
    app_coordinator_state_t state;
    bool battery_online;

    portENTER_CRITICAL(&s_status_mux);
    state = s_status.coordinator_state;
    battery_online = s_status.battery_identity_valid && s_status.battery_online;
    ++s_status.tx_attempts;
    portEXIT_CRITICAL(&s_status_mux);

    data[0] = s_heartbeat_counter++;
    data[1] = (uint8_t)state;
    data[2] = 0;
    data[3] = battery_online ? 1 : 0;
    uint32_t uptime_seconds = (uint32_t)(now_us / 1000000ULL);
    data[4] = (uint8_t)uptime_seconds;
    data[5] = (uint8_t)(uptime_seconds >> 8);
    data[6] = (uint8_t)(uptime_seconds >> 16);
    data[7] = (uint8_t)(uptime_seconds >> 24);

    twai_frame_t frame = {
        .header = {
            .id = CAN_ID_COORD_HEARTBEAT,
            .dlc = APP_CAN_MAX_DATA_LENGTH,
            .ide = false,
            .rtr = false,
        },
        .buffer = data,
        .buffer_len = sizeof(data),
    };
    esp_err_t error = twai_node_transmit(s_node, &frame, 0);
    if (error != ESP_OK) {
        portENTER_CRITICAL(&s_status_mux);
        ++s_status.tx_failures;
        portEXIT_CRITICAL(&s_status_mux);
        if (now_us - s_last_tx_error_log_us >= 1000000ULL) {
            ESP_LOGW(TAG, "Coordinator heartbeat queue failed: %s", esp_err_to_name(error));
            s_last_tx_error_log_us = now_us;
        }
    }
}

static void recover_bus_if_needed(void)
{
    twai_node_status_t node_status = {0};
    esp_err_t error = twai_node_get_info(s_node, &node_status, NULL);
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "TWAI status read failed: %s", esp_err_to_name(error));
        return;
    }

    if (node_status.state != TWAI_ERROR_BUS_OFF) {
        s_bus_recovery_started = false;
        return;
    }
    if (s_bus_recovery_started) {
        return;
    }

    error = twai_node_recover(s_node);
    if (error == ESP_OK || error == ESP_ERR_INVALID_STATE) {
        s_bus_recovery_started = true;
        ESP_LOGW(TAG, "TWAI bus-off recovery started");
        return;
    }
    ESP_LOGE(TAG, "TWAI bus-off recovery failed: %s", esp_err_to_name(error));
}

static void can_service_task(void *argument)
{
    (void)argument;
    can_rx_frame_t frame;
    uint64_t next_heartbeat_us = esp_timer_get_time();

    while (true) {
        uint64_t now_us = esp_timer_get_time();
        if (now_us >= next_heartbeat_us) {
            update_coordinator_state(now_us);
            transmit_coordinator_heartbeat(now_us);
            next_heartbeat_us += CAN_HEARTBEAT_PERIOD_US;
            if (next_heartbeat_us <= now_us) {
                next_heartbeat_us = now_us + CAN_HEARTBEAT_PERIOD_US;
            }
        }

        recover_bus_if_needed();
        now_us = esp_timer_get_time();
        uint64_t wait_us = next_heartbeat_us > now_us
                               ? next_heartbeat_us - now_us
                               : 0;
        TickType_t wait_ticks = pdMS_TO_TICKS((wait_us + 999ULL) / 1000ULL);
        if (wait_ticks == 0) {
            wait_ticks = 1;
        }

        if (xQueueReceive(s_rx_queue, &frame, wait_ticks) == pdTRUE) {
            uint64_t received_us = esp_timer_get_time();
            portENTER_CRITICAL(&s_status_mux);
            ++s_status.received_frames;
            s_status.last_identifier = frame.identifier;
            s_status.last_received_us = received_us;
            s_status.last_data_length = frame.data_length;
            memcpy(s_status.last_data, frame.data, sizeof(s_status.last_data));
            s_status.last_frame_extended = frame.extended;
            s_status.last_frame_remote = frame.remote;
            portEXIT_CRITICAL(&s_status_mux);
            process_battery_frame(&frame, received_us);
        }
    }
}

static esp_err_t fail_start(esp_err_t error, const char *operation)
{
    ESP_LOGE(TAG, "%s failed: %s", operation, esp_err_to_name(error));
    portENTER_CRITICAL(&s_status_mux);
    s_status.ready = false;
    s_status.init_error = error;
    portEXIT_CRITICAL(&s_status_mux);
    if (s_rx_queue != NULL) {
        vQueueDelete(s_rx_queue);
        s_rx_queue = NULL;
    }
    return error;
}

esp_err_t app_can_start(void)
{
    if (s_status.ready) {
        return ESP_ERR_INVALID_STATE;
    }

    s_rx_queue = xQueueCreate(CAN_RX_QUEUE_LENGTH, sizeof(can_rx_frame_t));
    if (s_rx_queue == NULL) {
        return fail_start(ESP_ERR_NO_MEM, "RX queue creation");
    }

    const twai_onchip_node_config_t node_config = {
        .io_cfg = {
            .tx = CAN_TX_GPIO,
            .rx = CAN_RX_GPIO,
            .quanta_clk_out = GPIO_NUM_NC,
            .bus_off_indicator = GPIO_NUM_NC,
        },
        .bit_timing = {
            .bitrate = CAN_BITRATE,
        },
        .fail_retry_cnt = 3,
        .tx_queue_depth = 4,
        .flags = {
            .no_receive_rtr = 1,
        },
    };
    esp_err_t error = twai_new_node_onchip(&node_config, &s_node);
    if (error != ESP_OK) {
        return fail_start(error, "TWAI node creation");
    }

    const twai_event_callbacks_t callbacks = {
        .on_tx_done = can_tx_done_callback,
        .on_rx_done = can_rx_done_callback,
        .on_state_change = can_state_change_callback,
    };
    error = twai_node_register_event_callbacks(s_node, &callbacks, s_rx_queue);
    if (error != ESP_OK) {
        twai_node_delete(s_node);
        s_node = NULL;
        return fail_start(error, "TWAI callback registration");
    }

    error = twai_node_enable(s_node);
    if (error != ESP_OK) {
        twai_node_delete(s_node);
        s_node = NULL;
        return fail_start(error, "TWAI node enable");
    }

    s_start_time_us = esp_timer_get_time();
    portENTER_CRITICAL(&s_status_mux);
    s_status.coordinator_state = APP_COORD_STATE_DISCOVERY;
    s_status.ready = true;
    s_status.init_error = ESP_OK;
    portEXIT_CRITICAL(&s_status_mux);

    if (xTaskCreatePinnedToCore(
            can_service_task,
            "can_service",
            CAN_SERVICE_TASK_STACK_SIZE,
            NULL,
            CAN_SERVICE_TASK_PRIORITY,
            NULL,
            1) != pdPASS) {
        twai_node_disable(s_node);
        twai_node_delete(s_node);
        s_node = NULL;
        return fail_start(ESP_ERR_NO_MEM, "CAN service task creation");
    }

    ESP_LOGI(TAG, "Bidirectional Classic CAN ready at %d bit/s", CAN_BITRATE);
    return ESP_OK;
}

void app_can_get_status(app_can_status_t *status)
{
    if (status == NULL) {
        return;
    }

    uint64_t now_us = esp_timer_get_time();
    portENTER_CRITICAL(&s_status_mux);
    *status = s_status;
    portEXIT_CRITICAL(&s_status_mux);

    status->battery_online =
        status->battery_heartbeat_us != 0 &&
        now_us - status->battery_heartbeat_us <= CAN_HEARTBEAT_TIMEOUT_US;
    status->battery_fast_telemetry_stale =
        !status->battery_fast_telemetry_valid ||
        now_us - status->battery_fast_telemetry_us > CAN_FAST_TELEMETRY_TIMEOUT_US;
    status->battery_slow_telemetry_stale =
        !status->battery_slow_telemetry_valid ||
        now_us - status->battery_slow_telemetry_us > CAN_SLOW_TELEMETRY_TIMEOUT_US;
}

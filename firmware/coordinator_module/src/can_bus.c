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
#define CAN_RX_TASK_STACK_SIZE 4096
#define CAN_RX_TASK_PRIORITY 8

static const char *TAG = "can_bus";
static portMUX_TYPE s_status_mux = portMUX_INITIALIZER_UNLOCKED;
static QueueHandle_t s_rx_queue;
static app_can_status_t s_status;
static twai_node_handle_t s_node;

typedef struct {
    uint32_t identifier;
    uint8_t data_length;
    uint8_t data[APP_CAN_MAX_DATA_LENGTH];
    bool extended;
    bool remote;
} can_rx_frame_t;

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

static void can_rx_task(void *argument)
{
    (void)argument;
    can_rx_frame_t frame;
    while (true) {
        if (xQueueReceive(s_rx_queue, &frame, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        uint64_t received_us = (uint64_t)esp_timer_get_time();
        portENTER_CRITICAL(&s_status_mux);
        ++s_status.received_frames;
        s_status.last_identifier = frame.identifier;
        s_status.last_received_us = received_us;
        s_status.last_data_length = frame.data_length;
        memcpy(s_status.last_data, frame.data, sizeof(s_status.last_data));
        s_status.last_frame_extended = frame.extended;
        s_status.last_frame_remote = frame.remote;
        portEXIT_CRITICAL(&s_status_mux);
    }
}

static esp_err_t fail_start(esp_err_t error, const char *operation)
{
    ESP_LOGE(TAG, "%s failed: %s", operation, esp_err_to_name(error));
    portENTER_CRITICAL(&s_status_mux);
    s_status.ready = false;
    s_status.init_error = error;
    portEXIT_CRITICAL(&s_status_mux);
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
        .tx_queue_depth = 1,
        .flags = {
            .no_receive_rtr = 1,
        },
    };
    esp_err_t error = twai_new_node_onchip(&node_config, &s_node);
    if (error != ESP_OK) {
        return fail_start(error, "TWAI node creation");
    }

    const twai_event_callbacks_t callbacks = {
        .on_rx_done = can_rx_done_callback,
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

    if (xTaskCreatePinnedToCore(
            can_rx_task,
            "can_rx",
            CAN_RX_TASK_STACK_SIZE,
            NULL,
            CAN_RX_TASK_PRIORITY,
            NULL,
            1) != pdPASS) {
        twai_node_disable(s_node);
        twai_node_delete(s_node);
        s_node = NULL;
        return fail_start(ESP_ERR_NO_MEM, "CAN RX task creation");
    }

    portENTER_CRITICAL(&s_status_mux);
    s_status.ready = true;
    s_status.init_error = ESP_OK;
    portEXIT_CRITICAL(&s_status_mux);
    ESP_LOGI(TAG, "Classic CAN receiver ready at %d bit/s", CAN_BITRATE);
    return ESP_OK;
}

void app_can_get_status(app_can_status_t *status)
{
    if (status == NULL) {
        return;
    }

    portENTER_CRITICAL(&s_status_mux);
    *status = s_status;
    portEXIT_CRITICAL(&s_status_mux);
}

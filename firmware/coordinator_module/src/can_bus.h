#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define APP_CAN_MAX_DATA_LENGTH 8

typedef struct {
    bool ready;
    esp_err_t init_error;
    uint32_t received_frames;
    uint32_t dropped_frames;
    uint32_t last_identifier;
    uint64_t last_received_us;
    uint8_t last_data_length;
    uint8_t last_data[APP_CAN_MAX_DATA_LENGTH];
    bool last_frame_extended;
    bool last_frame_remote;
} app_can_status_t;

esp_err_t app_can_start(void);
void app_can_get_status(app_can_status_t *status);

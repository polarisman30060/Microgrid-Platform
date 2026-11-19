#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define APP_CAN_MAX_DATA_LENGTH 8

typedef enum {
    APP_COORD_STATE_STARTUP = 0,
    APP_COORD_STATE_DISCOVERY = 1,
    APP_COORD_STATE_READY = 2,
    APP_COORD_STATE_DEGRADED = 5,
    APP_COORD_STATE_FAULT = 6,
} app_coordinator_state_t;

typedef struct {
    bool ready;
    esp_err_t init_error;
    uint32_t received_frames;
    uint32_t transmitted_frames;
    uint32_t tx_attempts;
    uint32_t tx_failures;
    uint32_t dropped_frames;
    uint32_t invalid_frames;
    uint32_t unexpected_frames;
    uint32_t last_identifier;
    uint64_t last_received_us;
    uint8_t last_data_length;
    uint8_t last_data[APP_CAN_MAX_DATA_LENGTH];
    bool last_frame_extended;
    bool last_frame_remote;
    app_coordinator_state_t coordinator_state;
    bool bus_off;
    bool battery_identity_valid;
    bool battery_online;
    uint32_t battery_serial_number;
    uint64_t battery_heartbeat_us;
    uint64_t battery_identity_us;
    uint8_t battery_local_state;
    uint8_t battery_warning_flags;
    uint16_t battery_aux_voltage_mv;
    uint16_t battery_firmware_build_id;
    bool battery_fast_telemetry_valid;
    bool battery_fast_telemetry_stale;
    uint64_t battery_fast_telemetry_us;
    float battery_bus_voltage_v;
    float battery_pack_voltage_v;
    float battery_current_a;
    float battery_soc_percent;
    uint8_t battery_mode_and_contactors;
    bool battery_slow_telemetry_valid;
    bool battery_slow_telemetry_stale;
    uint64_t battery_slow_telemetry_us;
    uint16_t battery_min_cell_voltage_mv;
    uint16_t battery_max_cell_voltage_mv;
    int8_t battery_max_temperature_c;
    int8_t battery_converter_temperature_c;
    float battery_available_capacity_ah;
} app_can_status_t;

esp_err_t app_can_start(void);
void app_can_get_status(app_can_status_t *status);

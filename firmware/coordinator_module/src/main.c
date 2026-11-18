#include <stdio.h>
#include <stdlib.h>

#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_lcd_touch.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "can_bus.h"
#include "waveshare_rgb_lcd_port.h"
#include "wifi_station.h"
#include "wifi_settings.h"

#define LVGL_BUFFER_LINES 40

static const char *TAG = "coordinator_ui";

#define COLOR_BACKGROUND 0x0A0A0C
#define COLOR_CARD 0x141418
#define COLOR_BORDER 0x26262B
#define COLOR_TEXT 0xF4F4F7
#define COLOR_MUTED 0x8E8E93
#define COLOR_SOLAR 0xEAB308
#define COLOR_GENERATOR 0xF97316
#define COLOR_GRID 0x06B6D4
#define COLOR_BUS 0x10B981
#define COLOR_BATTERY 0x3B82F6
#define COLOR_RAIL 0xA855F7
#define COLOR_TRIP 0xEF4444

static lv_obj_t *s_wifi_banner;
static lv_obj_t *s_can_status_label;
static lv_obj_t *s_can_frame_label;

static lv_obj_t *create_label(
    lv_obj_t *parent,
    const char *text,
    lv_coord_t x,
    lv_coord_t y,
    const lv_font_t *font,
    uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN);
    return label;
}

static void create_line(
    lv_obj_t *parent,
    lv_coord_t x,
    lv_coord_t y,
    lv_coord_t width,
    lv_coord_t height,
    uint32_t color)
{
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_set_pos(line, x, y);
    lv_obj_set_size(line, width, height);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(line, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line, 0, LV_PART_MAIN);
}

static lv_obj_t *create_card(
    lv_obj_t *parent,
    const char *title,
    const char *value,
    const char *detail,
    lv_coord_t x,
    lv_coord_t y,
    lv_coord_t width,
    lv_coord_t height,
    uint32_t accent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, width, height);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(card, lv_color_hex(COLOR_CARD), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_color_hex(accent), LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN);

    create_label(card, title, 9, 6, &lv_font_montserrat_14, accent);
    create_label(card, value, 9, 25, &lv_font_montserrat_20, COLOR_TEXT);
    create_label(card, detail, 9, height - 19, &lv_font_montserrat_14, COLOR_MUTED);
    return card;
}

static void create_disabled_button(lv_obj_t *parent, const char *text, lv_coord_t x, uint32_t accent)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_pos(button, x, 429);
    lv_obj_set_size(button, 92, 34);
    lv_obj_add_state(button, LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD), LV_PART_MAIN);
    lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD), LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_set_style_border_color(button, lv_color_hex(accent), LV_PART_MAIN);
    lv_obj_set_style_border_color(button, lv_color_hex(COLOR_BORDER), LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(button, 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(button, 0, LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_MUTED), LV_PART_MAIN);
    lv_obj_center(label);
}

static void open_wifi_settings_event(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        app_wifi_settings_show();
    }
}

static void wifi_banner_timer(lv_timer_t *timer)
{
    (void)timer;
    app_wifi_status_t status;
    if (app_wifi_station_get_status(&status) != ESP_OK) {
        lv_label_set_text(s_wifi_banner, "WI-FI STATUS UNAVAILABLE");
        return;
    }

    switch (status.state) {
    case APP_WIFI_STATE_CONNECTED:
        if (status.web_server_ready) {
            lv_label_set_text_fmt(s_wifi_banner, "WEB: http://%s/", status.ipv4);
            lv_obj_set_style_text_color(s_wifi_banner, lv_color_hex(COLOR_BUS), LV_PART_MAIN);
        } else {
            lv_label_set_text(s_wifi_banner, "WI-FI CONNECTED - WEB SERVER UNAVAILABLE");
            lv_obj_set_style_text_color(s_wifi_banner, lv_color_hex(COLOR_TRIP), LV_PART_MAIN);
        }
        break;
    case APP_WIFI_STATE_CONNECTING:
        lv_label_set_text(s_wifi_banner, "WI-FI: CONNECTING...");
        lv_obj_set_style_text_color(s_wifi_banner, lv_color_hex(COLOR_GRID), LV_PART_MAIN);
        break;
    case APP_WIFI_STATE_FAILED:
        lv_label_set_text(s_wifi_banner, "WI-FI: CONNECTION FAILED - OPEN SETUP");
        lv_obj_set_style_text_color(s_wifi_banner, lv_color_hex(COLOR_TRIP), LV_PART_MAIN);
        break;
    case APP_WIFI_STATE_STOPPED:
    case APP_WIFI_STATE_DISCONNECTED:
    default:
        lv_label_set_text(s_wifi_banner, "WI-FI: NOT CONNECTED - OPEN SETUP");
        lv_obj_set_style_text_color(s_wifi_banner, lv_color_hex(COLOR_MUTED), LV_PART_MAIN);
        break;
    }
}

static void can_status_timer(lv_timer_t *timer)
{
    (void)timer;
    app_can_status_t status;
    app_can_get_status(&status);

    if (!status.ready) {
        lv_label_set_text(s_can_status_label, "CAN ERROR");
        lv_obj_set_style_text_color(s_can_status_label, lv_color_hex(COLOR_TRIP), LV_PART_MAIN);
        lv_obj_set_style_border_color(
            lv_obj_get_parent(s_can_status_label), lv_color_hex(COLOR_TRIP), LV_PART_MAIN);
        lv_label_set_text_fmt(
            s_can_frame_label, "CAN initialization failed: %s",
            esp_err_to_name(status.init_error));
        return;
    }

    if (status.received_frames == 0) {
        lv_label_set_text(s_can_status_label, "CAN READY");
        lv_obj_set_style_text_color(s_can_status_label, lv_color_hex(COLOR_GRID), LV_PART_MAIN);
        lv_obj_set_style_border_color(
            lv_obj_get_parent(s_can_status_label), lv_color_hex(COLOR_GRID), LV_PART_MAIN);
        lv_label_set_text(s_can_frame_label, "CAN ready at 500 kbps; waiting for frames");
        return;
    }

    lv_label_set_text(s_can_status_label, "CAN RX");
    lv_obj_set_style_text_color(s_can_status_label, lv_color_hex(COLOR_BUS), LV_PART_MAIN);
    lv_obj_set_style_border_color(
        lv_obj_get_parent(s_can_status_label), lv_color_hex(COLOR_BUS), LV_PART_MAIN);

    char frame_text[120];
    uint64_t age_ms = ((uint64_t)esp_timer_get_time() - status.last_received_us) / 1000;
    size_t used = (size_t)snprintf(
        frame_text, sizeof(frame_text), "RX %lu | %llums ago | ID 0x%03lX%s | DLC %u |",
        (unsigned long)status.received_frames,
        (unsigned long long)age_ms,
        (unsigned long)status.last_identifier,
        status.last_frame_extended ? " EXT" : "",
        (unsigned)status.last_data_length);
    for (size_t i = 0; i < status.last_data_length && used < sizeof(frame_text); ++i) {
        int written = snprintf(
            frame_text + used, sizeof(frame_text) - used, " %02X",
            (unsigned)status.last_data[i]);
        if (written < 0 || (size_t)written >= sizeof(frame_text) - used) {
            break;
        }
        used += (size_t)written;
    }
    lv_label_set_text(s_can_frame_label, frame_text);
}

static void lcd_flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *pixels)
{
    esp_lcd_panel_handle_t panel = driver->user_data;
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
        panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, pixels));
    lv_disp_flush_ready(driver);
}

static void touch_read(lv_indev_drv_t *driver, lv_indev_data_t *data)
{
    esp_lcd_touch_handle_t touch = driver->user_data;
    esp_err_t err = esp_lcd_touch_read_data(touch);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Touch read failed: %s", esp_err_to_name(err));
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    esp_lcd_touch_point_data_t point = {0};
    uint8_t point_count = 0;
    err = esp_lcd_touch_get_data(touch, &point, &point_count, 1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Touch coordinate read failed: %s", esp_err_to_name(err));
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    if (point_count > 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = point.x;
        data->point.y = point.y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static lv_obj_t *create_dashboard(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    create_label(screen, "MICROGRID ORCHESTRATION", 12, 9, &lv_font_montserrat_20, COLOR_TEXT);
    create_label(screen, "DYNAMIC TOPOLOGY  -  SYSTEM OVERVIEW", 13, 36, &lv_font_montserrat_14, COLOR_MUTED);
    create_line(screen, 12, 65, 776, 1, COLOR_BORDER);
    s_can_frame_label = create_label(
        screen, "CAN: waiting for frames at 500 kbps", 12, 72,
        &lv_font_montserrat_14, COLOR_MUTED);
    lv_obj_set_width(s_can_frame_label, 776);
    lv_label_set_long_mode(s_can_frame_label, LV_LABEL_LONG_DOT);

    lv_obj_t *status = lv_obj_create(screen);
    lv_obj_set_pos(status, 665, 17);
    lv_obj_set_size(status, 123, 32);
    lv_obj_clear_flag(status, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(status, lv_color_hex(0x261416), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(status, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(status, lv_color_hex(COLOR_TRIP), LV_PART_MAIN);
    lv_obj_set_style_border_width(status, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(status, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_all(status, 0, LV_PART_MAIN);
    s_can_status_label = lv_label_create(status);
    lv_label_set_text(s_can_status_label, "CAN STARTING");
    lv_obj_set_style_text_font(s_can_status_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_can_status_label, lv_color_hex(COLOR_TRIP), LV_PART_MAIN);
    lv_obj_center(s_can_status_label);

    lv_obj_t *setup_button = lv_btn_create(screen);
    lv_obj_set_pos(setup_button, 535, 17);
    lv_obj_set_size(setup_button, 112, 32);
    lv_obj_set_style_bg_color(setup_button, lv_color_hex(COLOR_CARD), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(setup_button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(setup_button, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(setup_button, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(setup_button, 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(setup_button, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(setup_button, open_wifi_settings_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *setup_label = lv_label_create(setup_button);
    lv_label_set_text(setup_label, "WI-FI SETUP");
    lv_obj_set_style_text_font(setup_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(setup_label, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_obj_center(setup_label);

    create_line(screen, 145, 134, 18, 3, COLOR_BORDER);
    create_line(screen, 145, 214, 18, 3, COLOR_BORDER);
    create_line(screen, 145, 294, 18, 3, COLOR_BORDER);
    create_line(screen, 163, 134, 6, 160, COLOR_BUS);
    create_line(screen, 169, 214, 306, 4, COLOR_BUS);

    create_line(screen, 398, 214, 4, 121, COLOR_BATTERY);
    create_line(screen, 625, 214, 7, 4, COLOR_RAIL);
    create_line(screen, 632, 174, 4, 85, COLOR_RAIL);
    create_line(screen, 636, 174, 4, 3, COLOR_BORDER);
    create_line(screen, 636, 254, 4, 3, COLOR_BORDER);

    create_card(screen, "SOLAR ARRAY", "-- kW", "Waiting for telemetry", 10, 100, 135, 68, COLOR_SOLAR);
    create_card(screen, "DIESEL GEN", "-- kW", "Waiting for telemetry", 10, 180, 135, 68, COLOR_GENERATOR);
    create_card(screen, "UTILITY GRID", "-- kW", "Waiting for telemetry", 10, 260, 135, 68, COLOR_GRID);

    lv_obj_t *bus_96 = lv_obj_create(screen);
    lv_obj_set_pos(bus_96, 142, 309);
    lv_obj_set_size(bus_96, 88, 23);
    lv_obj_clear_flag(bus_96, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(bus_96, lv_color_hex(COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bus_96, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(bus_96, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bus_96, 0, LV_PART_MAIN);
    lv_obj_t *bus_96_label = lv_label_create(bus_96);
    lv_label_set_text(bus_96_label, "96VDC BUS");
    lv_obj_set_style_text_font(bus_96_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(bus_96_label, lv_color_hex(COLOR_BUS), LV_PART_MAIN);
    lv_obj_center(bus_96_label);

    create_card(screen, "96V TO 380V", "-- kW", "No active node telemetry", 475, 174, 150, 82, COLOR_RAIL);
    create_card(screen, "STORAGE ARRAY", "-- kW", "Waiting for telemetry", 290, 335, 220, 77, COLOR_BATTERY);

    lv_obj_t *bus_380 = lv_obj_create(screen);
    lv_obj_set_pos(bus_380, 590, 284);
    lv_obj_set_size(bus_380, 92, 23);
    lv_obj_clear_flag(bus_380, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(bus_380, lv_color_hex(COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bus_380, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(bus_380, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bus_380, 0, LV_PART_MAIN);
    lv_obj_t *bus_380_label = lv_label_create(bus_380);
    lv_label_set_text(bus_380_label, "380VDC BUS");
    lv_obj_set_style_text_font(bus_380_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(bus_380_label, lv_color_hex(COLOR_RAIL), LV_PART_MAIN);
    lv_obj_center(bus_380_label);

    create_card(screen, "120V INVERTER", "-- kW", "Waiting for telemetry", 640, 140, 150, 68, COLOR_BORDER);
    create_card(screen, "240V SPLIT PHASE", "-- kW", "Waiting for telemetry", 640, 220, 150, 68, COLOR_BORDER);

    create_line(screen, 12, 420, 776, 1, COLOR_BORDER);
    s_wifi_banner = create_label(
        screen, "WI-FI: NOT CONNECTED - OPEN SETUP", 14, 438,
        &lv_font_montserrat_14, COLOR_MUTED);
    create_disabled_button(screen, "START", 580, COLOR_BUS);
    create_disabled_button(screen, "STOP", 686, COLOR_TRIP);
    return screen;
}

void app_main(void)
{
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_touch_handle_t touch_handle = NULL;
    ESP_ERROR_CHECK(waveshare_rgb_lcd_init(2, &panel_handle, &touch_handle));

    lv_init();
    const size_t buffer_pixels = LCD_H_RES * LVGL_BUFFER_LINES;
    lv_color_t *buffer = heap_caps_malloc(
        buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Could not allocate LVGL draw buffer in PSRAM");
        abort();
    }

    static lv_disp_draw_buf_t draw_buffer;
    lv_disp_draw_buf_init(&draw_buffer, buffer, NULL, buffer_pixels);

    static lv_disp_drv_t display_driver;
    lv_disp_drv_init(&display_driver);
    display_driver.hor_res = LCD_H_RES;
    display_driver.ver_res = LCD_V_RES;
    display_driver.flush_cb = lcd_flush;
    display_driver.draw_buf = &draw_buffer;
    display_driver.user_data = panel_handle;
    if (lv_disp_drv_register(&display_driver) == NULL) {
        ESP_LOGE(TAG, "Could not register LVGL display");
        abort();
    }

    static lv_indev_drv_t touch_driver;
    lv_indev_drv_init(&touch_driver);
    touch_driver.type = LV_INDEV_TYPE_POINTER;
    touch_driver.read_cb = touch_read;
    touch_driver.user_data = touch_handle;
    if (lv_indev_drv_register(&touch_driver) == NULL) {
        ESP_LOGE(TAG, "Could not register LVGL touch input");
        abort();
    }

    lv_obj_t *dashboard = create_dashboard();
    lv_scr_load(dashboard);

    ESP_LOGI(TAG, "Display ready");
    ESP_ERROR_CHECK(app_wifi_station_start());
    app_wifi_settings_create(dashboard);
    esp_err_t can_error = app_can_start();
    if (can_error != ESP_OK) {
        ESP_LOGE(TAG, "CAN receiver unavailable: %s", esp_err_to_name(can_error));
    }
    lv_timer_create(wifi_banner_timer, 1000, NULL);
    lv_timer_create(can_status_timer, 250, NULL);
    while (true) {
        lv_tick_inc(5);
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
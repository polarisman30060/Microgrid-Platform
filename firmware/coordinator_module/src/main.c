#include <stdlib.h>

#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_lcd_touch.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "waveshare_rgb_lcd_port.h"
#include "wifi_station.h"

#define LVGL_BUFFER_LINES 40

static const char *TAG = "hello_world";

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

static void greeting_clicked(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Touch input detected");
    }
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

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello Alicia");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label, greeting_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_center(label);

    ESP_LOGI(TAG, "Display ready");
    ESP_ERROR_CHECK(app_wifi_station_start());
    while (true) {
        lv_tick_inc(5);
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
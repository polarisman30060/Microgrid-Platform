#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

#define LCD_H_RES 800
#define LCD_V_RES 480

esp_err_t waveshare_rgb_lcd_init(
    uint8_t num_fbs,
    esp_lcd_panel_handle_t *panel_handle,
    esp_lcd_touch_handle_t *touch_handle);

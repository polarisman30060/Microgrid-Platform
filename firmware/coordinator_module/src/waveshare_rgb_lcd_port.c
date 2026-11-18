#include "waveshare_rgb_lcd_port.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#define I2C_SDA GPIO_NUM_8
#define I2C_SCL GPIO_NUM_9
#define I2C_FREQ_HZ 400000

static const char *TAG = "lcd";
static i2c_master_bus_handle_t s_i2c_bus;
static i2c_master_dev_handle_t s_ch422g_mode;
static i2c_master_dev_handle_t s_ch422g_output;

static esp_err_t init_i2c(void)
{
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &s_i2c_bus), TAG, "I2C bus init failed");

    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    i2c_device_config_t mode_config = device_config;
    mode_config.device_address = 0x24;
    ESP_RETURN_ON_ERROR(
        i2c_master_bus_add_device(s_i2c_bus, &mode_config, &s_ch422g_mode),
        TAG, "CH422G mode device init failed");

    i2c_device_config_t output_config = device_config;
    output_config.device_address = 0x38;
    return i2c_master_bus_add_device(s_i2c_bus, &output_config, &s_ch422g_output);
}

static esp_err_t init_touch_reset(void)
{
    const uint64_t reset_pin_mask = 1ULL << GPIO_NUM_4;
    const gpio_config_t reset_gpio_config = {
        .pin_bit_mask = reset_pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&reset_gpio_config), TAG, "Touch reset GPIO init failed");

    const uint8_t output_mode = 0x01;
    uint8_t expander_output = 0x2C;
    ESP_RETURN_ON_ERROR(
        i2c_master_transmit(s_ch422g_mode, &output_mode, 1, 1000),
        TAG, "CH422G output mode setup failed");
    ESP_RETURN_ON_ERROR(
        i2c_master_transmit(s_ch422g_output, &expander_output, 1, 1000),
        TAG, "Touch reset assertion failed");
    esp_rom_delay_us(100 * 1000);
    ESP_RETURN_ON_ERROR(gpio_set_level(GPIO_NUM_4, 0), TAG, "Touch reset GPIO write failed");
    esp_rom_delay_us(100 * 1000);
    expander_output = 0x2E;
    ESP_RETURN_ON_ERROR(
        i2c_master_transmit(s_ch422g_output, &expander_output, 1, 1000),
        TAG, "Touch reset release failed");
    esp_rom_delay_us(200 * 1000);
    return ESP_OK;
}

static esp_err_t init_touch(esp_lcd_touch_handle_t *touch_handle)
{
    ESP_RETURN_ON_ERROR(init_touch_reset(), TAG, "Touch reset failed");

    esp_lcd_panel_io_handle_t touch_io_handle = NULL;
    const esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_i2c(s_i2c_bus, &io_config, &touch_io_handle),
        TAG, "Touch panel I2C init failed");

    const esp_lcd_touch_config_t touch_config = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    return esp_lcd_touch_new_i2c_gt911(touch_io_handle, &touch_config, touch_handle);
}

static esp_err_t enable_backlight(void)
{
    const uint8_t output_mode = 0x01;
    const uint8_t backlight_on_and_can_selected = 0x3E;

    ESP_RETURN_ON_ERROR(
        i2c_master_transmit(s_ch422g_mode, &output_mode, 1, 1000),
        TAG, "CH422G output mode setup failed");
    return i2c_master_transmit(
        s_ch422g_output, &backlight_on_and_can_selected, 1, 1000);
}

esp_err_t waveshare_rgb_lcd_init(
    uint8_t num_fbs,
    esp_lcd_panel_handle_t *panel_handle,
    esp_lcd_touch_handle_t *touch_handle)
{
    if (panel_handle == NULL || touch_handle == NULL || num_fbs == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    *panel_handle = NULL;
    *touch_handle = NULL;

    ESP_LOGI(TAG, "Initializing shared I2C bus");
    ESP_RETURN_ON_ERROR(init_i2c(), TAG, "I2C init failed");

    const esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = 16 * 1000 * 1000,
            .h_res = LCD_H_RES,
            .v_res = LCD_V_RES,
            .hsync_pulse_width = 4,
            .hsync_back_porch = 8,
            .hsync_front_porch = 8,
            .vsync_pulse_width = 4,
            .vsync_back_porch = 8,
            .vsync_front_porch = 8,
            .flags.pclk_active_neg = 1,
        },
        .data_width = 16,
        .in_color_format = LCD_COLOR_FMT_RGB565,
        .out_color_format = LCD_COLOR_FMT_RGB565,
        .num_fbs = num_fbs,
        .bounce_buffer_size_px = LCD_H_RES * 10,
        .hsync_gpio_num = GPIO_NUM_46,
        .vsync_gpio_num = GPIO_NUM_3,
        .de_gpio_num = GPIO_NUM_5,
        .pclk_gpio_num = GPIO_NUM_7,
        .disp_gpio_num = -1,
        .data_gpio_nums = {
            GPIO_NUM_14, GPIO_NUM_38, GPIO_NUM_18, GPIO_NUM_17,
            GPIO_NUM_10, GPIO_NUM_39, GPIO_NUM_0, GPIO_NUM_45,
            GPIO_NUM_48, GPIO_NUM_47, GPIO_NUM_21, GPIO_NUM_1,
            GPIO_NUM_2, GPIO_NUM_42, GPIO_NUM_41, GPIO_NUM_40,
        },
        .flags.fb_in_psram = 1,
    };

    ESP_LOGI(TAG, "Initializing 800x480 RGB panel");
    ESP_RETURN_ON_ERROR(esp_lcd_new_rgb_panel(&panel_config, panel_handle), TAG, "Panel create failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(*panel_handle), TAG, "Panel init failed");
    ESP_LOGI(TAG, "Initializing GT911 touchscreen");
    ESP_RETURN_ON_ERROR(init_touch(touch_handle), TAG, "Touch init failed");
    ESP_RETURN_ON_ERROR(enable_backlight(), TAG, "Backlight enable failed");
    return ESP_OK;
}

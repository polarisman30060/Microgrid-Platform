#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "lvgl.h"

#include "wifi_settings.h"
#include "wifi_station.h"

#define COLOR_BACKGROUND 0x0A0A0C
#define COLOR_CARD 0x141418
#define COLOR_BORDER 0x26262B
#define COLOR_TEXT 0xF4F4F7
#define COLOR_MUTED 0x8E8E93
#define COLOR_ACCENT 0x06B6D4
#define COLOR_SUCCESS 0x10B981
#define COLOR_ERROR 0xEF4444

#define WIFI_SETTINGS_SCAN_LIMIT APP_WIFI_MAX_SCAN_RESULTS

static const char *TAG = "wifi_settings";
static lv_obj_t *s_dashboard;
static lv_obj_t *s_network_screen;
static lv_obj_t *s_credentials_screen;
static lv_obj_t *s_network_list;
static lv_obj_t *s_connection_label;
static lv_obj_t *s_scan_label;
static lv_obj_t *s_credentials_message;
static lv_obj_t *s_ssid_field;
static lv_obj_t *s_password_field;
static lv_obj_t *s_keyboard_panel;
static lv_obj_t *s_active_textarea;
static lv_obj_t *s_keyboard_buttons[3][10];
static size_t s_keyboard_button_counts[3];
static lv_obj_t *s_keyboard_mode_button;
static lv_obj_t *s_keyboard_shift_button;
static bool s_keyboard_symbols;
static bool s_keyboard_uppercase;
static app_wifi_access_point_t s_access_points[WIFI_SETTINGS_SCAN_LIMIT];
static size_t s_access_point_count;
static bool s_scan_rendered;

static void network_selected_event(lv_event_t *event);
static void keyboard_key_event(lv_event_t *event);

static void show_keyboard(lv_obj_t *textarea)
{
    s_active_textarea = textarea;
    lv_obj_clear_flag(s_keyboard_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_keyboard_panel);
}

static lv_obj_t *create_label(
    lv_obj_t *parent,
    const char *text,
    lv_coord_t x,
    lv_coord_t y,
    lv_coord_t width,
    const lv_font_t *font,
    uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, width);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN);
    return label;
}

static lv_obj_t *create_button(
    lv_obj_t *parent,
    const char *text,
    lv_coord_t x,
    lv_coord_t y,
    lv_coord_t width,
    lv_coord_t height,
    lv_event_cb_t callback,
    void *user_data)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, width, height);
    lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(button, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(button, 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(button, 0, LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_obj_center(label);

    if (callback != NULL) {
        lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user_data);
    }
    return button;
}

static void set_keyboard_button_label(lv_obj_t *button, const char *text)
{
    lv_label_set_text(lv_obj_get_child(button, 0), text);
}

static void update_keyboard_case(void)
{
    static const char *const letters[] = {
        "q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
        "a", "s", "d", "f", "g", "h", "j", "k", "l",
        "z", "x", "c", "v", "b", "n", "m",
    };
    size_t letter_index = 0;
    for (size_t row = 0; row < 3; ++row) {
        for (size_t key = 0; key < s_keyboard_button_counts[row]; ++key) {
            const char *current = lv_label_get_text(
                lv_obj_get_child(s_keyboard_buttons[row][key], 0));
            if (strcmp(current, "SHIFT") == 0 || strcmp(current, "SHIFT ON") == 0) {
                s_keyboard_shift_button = s_keyboard_buttons[row][key];
                continue;
            }
            if (strlen(current) == 1 && letter_index < sizeof(letters) / sizeof(letters[0])) {
                char label[2] = {letters[letter_index][0], '\0'};
                if (s_keyboard_uppercase) {
                    label[0] = (char)(label[0] - 'a' + 'A');
                }
                set_keyboard_button_label(s_keyboard_buttons[row][key], label);
                ++letter_index;
            }
        }
    }

    if (s_keyboard_shift_button != NULL) {
        set_keyboard_button_label(
            s_keyboard_shift_button, s_keyboard_uppercase ? "SHIFT ON" : "SHIFT");
    }
}

static void set_keyboard_mode(bool symbols)
{
    static const char *const alpha_rows[3][10] = {
        {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p"},
        {"a", "s", "d", "f", "g", "h", "j", "k", "l"},
        {"SHIFT", "z", "x", "c", "v", "b", "n", "m", "DEL"},
    };
    static const char *const symbol_rows[3][10] = {
        {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
        {"!", "@", "#", "$", "%", "^", "&", "*", "("},
        {")", "+", "-", "=", "_", "/", "\\", ":", "DEL"},
    };
    const char *const (*rows)[10] = symbols ? symbol_rows : alpha_rows;

    s_keyboard_symbols = symbols;
    s_keyboard_uppercase = false;
    for (size_t row = 0; row < 3; ++row) {
        for (size_t key = 0; key < s_keyboard_button_counts[row]; ++key) {
            set_keyboard_button_label(s_keyboard_buttons[row][key], rows[row][key]);
        }
    }
    set_keyboard_button_label(s_keyboard_mode_button, symbols ? "ABC" : "123");
    s_keyboard_shift_button = NULL;
    if (!symbols) {
        update_keyboard_case();
    }
}

static lv_obj_t *create_keyboard_key(
    lv_obj_t *parent,
    const char *label,
    lv_coord_t x,
    lv_coord_t y,
    lv_coord_t width)
{
    return create_button(
        parent, label, x, y, width, 50, keyboard_key_event, NULL);
}

static void create_keyboard_layout(void)
{
    s_keyboard_panel = lv_obj_create(s_credentials_screen);
    lv_obj_set_pos(s_keyboard_panel, 10, 230);
    lv_obj_set_size(s_keyboard_panel, 780, 240);
    lv_obj_clear_flag(s_keyboard_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_keyboard_panel, lv_color_hex(COLOR_CARD), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_keyboard_panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_keyboard_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_keyboard_panel, 0, LV_PART_MAIN);

    static const char *const row_one[] = {
        "q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
    };
    static const char *const row_two[] = {
        "a", "s", "d", "f", "g", "h", "j", "k", "l",
    };
    static const char *const row_three[] = {
        "SHIFT", "z", "x", "c", "v", "b", "n", "m", "DEL",
    };
    for (size_t i = 0; i < sizeof(row_one) / sizeof(row_one[0]); ++i) {
        s_keyboard_buttons[0][i] = create_keyboard_key(
            s_keyboard_panel, row_one[i], (lv_coord_t)(2 + i * 78), 6, 74);
    }
    s_keyboard_button_counts[0] = sizeof(row_one) / sizeof(row_one[0]);
    for (size_t i = 0; i < sizeof(row_two) / sizeof(row_two[0]); ++i) {
        s_keyboard_buttons[1][i] = create_keyboard_key(
            s_keyboard_panel, row_two[i], (lv_coord_t)(41 + i * 78), 62, 74);
    }
    s_keyboard_button_counts[1] = sizeof(row_two) / sizeof(row_two[0]);
    for (size_t i = 0; i < sizeof(row_three) / sizeof(row_three[0]); ++i) {
        lv_coord_t x = i == 0 ? 15 : (i == 8 ? 665 : (lv_coord_t)(119 + (i - 1) * 78));
        lv_coord_t width = (i == 0 || i == 8) ? 100 : 74;
        s_keyboard_buttons[2][i] = create_keyboard_key(
            s_keyboard_panel, row_three[i], x, 118, width);
    }
    s_keyboard_button_counts[2] = sizeof(row_three) / sizeof(row_three[0]);

    s_keyboard_mode_button = create_keyboard_key(s_keyboard_panel, "123", 12, 174, 100);
    create_keyboard_key(s_keyboard_panel, ",", 116, 174, 70);
    create_keyboard_key(s_keyboard_panel, "SPACE", 190, 174, 400);
    create_keyboard_key(s_keyboard_panel, ".", 594, 174, 70);
    create_keyboard_key(s_keyboard_panel, "DONE", 668, 174, 100);

    s_keyboard_shift_button = s_keyboard_buttons[2][0];
    lv_obj_add_flag(s_keyboard_panel, LV_OBJ_FLAG_HIDDEN);
}

static void keyboard_key_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || s_active_textarea == NULL) {
        return;
    }

    lv_obj_t *button = lv_event_get_target(event);
    const char *key = lv_label_get_text(lv_obj_get_child(button, 0));
    if (strcmp(key, "SHIFT") == 0 || strcmp(key, "SHIFT ON") == 0) {
        s_keyboard_uppercase = !s_keyboard_uppercase;
        update_keyboard_case();
    } else if (strcmp(key, "123") == 0) {
        set_keyboard_mode(true);
    } else if (strcmp(key, "ABC") == 0) {
        set_keyboard_mode(false);
    } else if (strcmp(key, "DEL") == 0) {
        lv_textarea_del_char(s_active_textarea);
    } else if (strcmp(key, "SPACE") == 0) {
        lv_textarea_add_char(s_active_textarea, ' ');
    } else if (strcmp(key, "DONE") == 0) {
        lv_obj_add_flag(s_keyboard_panel, LV_OBJ_FLAG_HIDDEN);
    } else if (key[0] != '\0' && key[1] == '\0') {
        lv_textarea_add_char(s_active_textarea, (uint32_t)(unsigned char)key[0]);
    }
}

static void set_screen_background(lv_obj_t *screen)
{
    lv_obj_set_style_bg_color(screen, lv_color_hex(COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

static void update_connection_label(void)
{
    app_wifi_status_t status;
    if (app_wifi_station_get_status(&status) != ESP_OK) {
        lv_label_set_text(s_connection_label, "Unable to read Wi-Fi status.");
        return;
    }

    char text[160];
    uint32_t color = COLOR_MUTED;
    switch (status.state) {
    case APP_WIFI_STATE_CONNECTED:
        if (!status.web_server_ready) {
            snprintf(text, sizeof(text), "Connected to %s, but the web server is not ready.",
                     status.ssid);
            color = COLOR_ERROR;
        } else if (status.credentials_saved) {
            snprintf(text, sizeof(text), "Connected to %s. Open http://%s/ on this network.",
                     status.ssid, status.ipv4);
            color = COLOR_SUCCESS;
        } else {
            snprintf(text, sizeof(text), "Connected to %s, but credentials could not be saved: %s.",
                     status.ssid, esp_err_to_name(status.last_error));
            color = COLOR_ERROR;
        }
        break;
    case APP_WIFI_STATE_CONNECTING:
        snprintf(text, sizeof(text), "Connecting to %s...", status.ssid);
        color = COLOR_ACCENT;
        break;
    case APP_WIFI_STATE_FAILED:
        if (status.disconnect_reason != 0) {
            snprintf(text, sizeof(text),
                     "Connection failed (Wi-Fi reason %u). Check the network and password, then try again.",
                     status.disconnect_reason);
        } else if (status.last_error != ESP_OK) {
            snprintf(text, sizeof(text), "Connection failed: %s. Try again.",
                     esp_err_to_name(status.last_error));
        } else {
            snprintf(text, sizeof(text),
                     "Connection failed. Check the network and password, then try again.");
        }
        color = COLOR_ERROR;
        break;
    case APP_WIFI_STATE_DISCONNECTED:
    case APP_WIFI_STATE_STOPPED:
    default:
        if (status.last_error != ESP_OK) {
            snprintf(text, sizeof(text), "Not connected. Wi-Fi settings could not be loaded: %s.",
                     esp_err_to_name(status.last_error));
            color = COLOR_ERROR;
        } else {
            snprintf(text, sizeof(text),
                     "Not connected. Choose a network or enter its name manually.");
        }
        break;
    }
    lv_label_set_text(s_connection_label, text);
    lv_obj_set_style_text_color(s_connection_label, lv_color_hex(color), LV_PART_MAIN);
}

static void render_access_points(void)
{
    lv_obj_clean(s_network_list);
    if (s_access_point_count == 0) {
        lv_obj_t *empty_label = lv_label_create(s_network_list);
        lv_label_set_text(empty_label, "No networks found. Try scanning again or enter the SSID manually.");
        lv_obj_set_style_text_font(empty_label, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(empty_label, lv_color_hex(COLOR_MUTED), LV_PART_MAIN);
        lv_obj_align(empty_label, LV_ALIGN_TOP_LEFT, 12, 12);
        return;
    }

    for (size_t i = 0; i < s_access_point_count; ++i) {
        lv_obj_t *button = lv_btn_create(s_network_list);
        lv_obj_set_pos(button, 0, (lv_coord_t)(i * 42));
        lv_obj_set_size(button, 744, 38);
        lv_obj_set_style_bg_color(button, lv_color_hex(COLOR_CARD), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(button, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(button, 5, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(button, 0, LV_PART_MAIN);
        char display_name[APP_WIFI_SSID_MAX_LENGTH + 1];
        size_t name_length = strnlen(
            s_access_points[i].ssid, sizeof(s_access_points[i].ssid));
        for (size_t j = 0; j < name_length; ++j) {
            unsigned char character = (unsigned char)s_access_points[i].ssid[j];
            display_name[j] = character >= 32 && character <= 126 ? (char)character : '?';
        }
        display_name[name_length] = '\0';

        char row_text[80];
        snprintf(row_text, sizeof(row_text), "%s   %d dBm   %s",
                 display_name, s_access_points[i].rssi,
                 s_access_points[i].secure ? "SECURED" : "OPEN");
        lv_obj_t *label = lv_label_create(button);
        lv_label_set_text(label, row_text);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 12, 0);

        lv_obj_add_event_cb(
            button, network_selected_event, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }
}

static void open_credentials(const char *ssid)
{
    lv_textarea_set_text(s_ssid_field, ssid);
    lv_textarea_set_text(s_password_field, "");
    lv_label_set_text(s_credentials_message, "Enter the Wi-Fi password. Leave it blank for an open network.");
    lv_obj_set_style_text_color(s_credentials_message, lv_color_hex(COLOR_MUTED), LV_PART_MAIN);
    lv_scr_load(s_credentials_screen);
    show_keyboard(s_password_field);
}

static void back_to_dashboard_event(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        lv_scr_load(s_dashboard);
    }
}

static void back_to_networks_event(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        lv_obj_add_flag(s_keyboard_panel, LV_OBJ_FLAG_HIDDEN);
        s_active_textarea = NULL;
        lv_scr_load(s_network_screen);
        update_connection_label();
    }
}

static void network_selected_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    size_t index = (size_t)(uintptr_t)lv_event_get_user_data(event);
    if (index >= s_access_point_count) {
        ESP_LOGE(TAG, "Selected network index is no longer valid");
        return;
    }
    open_credentials(s_access_points[index].ssid);
}

static void scan_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    s_scan_rendered = false;
    lv_label_set_text(s_scan_label, "Scanning for networks...");
    esp_err_t err = app_wifi_station_scan();
    if (err == ESP_ERR_NOT_FINISHED) {
        lv_label_set_text(s_scan_label, "A network scan is already in progress...");
    } else if (err != ESP_OK) {
        lv_label_set_text_fmt(s_scan_label, "Scan could not start: %s", esp_err_to_name(err));
        s_scan_rendered = true;
    }
}

static void manual_network_event(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        open_credentials("");
        show_keyboard(s_ssid_field);
    }
}

static void text_area_event(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED ||
        lv_event_get_code(event) == LV_EVENT_FOCUSED) {
        show_keyboard(lv_event_get_target(event));
    }
}

static void connect_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    const char *ssid = lv_textarea_get_text(s_ssid_field);
    const char *password = lv_textarea_get_text(s_password_field);
    esp_err_t err = app_wifi_station_connect(ssid, password);
    if (err != ESP_OK) {
        if (err == ESP_ERR_INVALID_SIZE) {
            lv_label_set_text(s_credentials_message, "Network name must be 1-32 characters; password may be up to 64.");
        } else if (err == ESP_ERR_INVALID_ARG) {
            lv_label_set_text(s_credentials_message, "Password must be blank or at least 8 characters.");
        } else {
            lv_label_set_text_fmt(
                s_credentials_message, "Could not start connection: %s", esp_err_to_name(err));
        }
        lv_obj_set_style_text_color(s_credentials_message, lv_color_hex(COLOR_ERROR), LV_PART_MAIN);
        return;
    }

    lv_textarea_set_text(s_password_field, "");
    lv_obj_add_flag(s_keyboard_panel, LV_OBJ_FLAG_HIDDEN);
    s_active_textarea = NULL;
    lv_scr_load(s_network_screen);
    update_connection_label();
    lv_label_set_text(s_scan_label, "Connection request sent.");
}

static void settings_timer(lv_timer_t *timer)
{
    (void)timer;
    update_connection_label();

    if (s_scan_rendered) {
        return;
    }

    size_t count = 0;
    esp_err_t err = app_wifi_station_get_scan_results(
        s_access_points, WIFI_SETTINGS_SCAN_LIMIT, &count);
    if (err == ESP_ERR_NOT_FINISHED) {
        return;
    }
    if (err != ESP_OK) {
        lv_label_set_text_fmt(s_scan_label, "Could not read networks: %s", esp_err_to_name(err));
        s_scan_rendered = true;
        return;
    }

    s_access_point_count = count;
    render_access_points();
    lv_label_set_text_fmt(s_scan_label, "%u network(s) found.", (unsigned)count);
    s_scan_rendered = true;
}

lv_obj_t *app_wifi_settings_create(lv_obj_t *dashboard)
{
    s_dashboard = dashboard;
    s_network_screen = lv_obj_create(NULL);
    set_screen_background(s_network_screen);

    lv_obj_t *back_button = create_button(
        s_network_screen, "BACK", 12, 12, 86, 38, back_to_dashboard_event, NULL);
    lv_obj_set_style_border_color(back_button, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
    create_label(s_network_screen, "WI-FI NETWORK SETUP", 116, 17, 430,
                 &lv_font_montserrat_20, COLOR_TEXT);

    s_connection_label = create_label(
        s_network_screen, "Checking Wi-Fi status...", 20, 66, 760,
        &lv_font_montserrat_14, COLOR_MUTED);
    lv_label_set_long_mode(s_connection_label, LV_LABEL_LONG_WRAP);
    create_label(s_network_screen, "AVAILABLE NETWORKS", 20, 119, 300,
                 &lv_font_montserrat_14, COLOR_MUTED);

    create_button(s_network_screen, "ENTER SSID", 492, 108, 132, 38,
                  manual_network_event, NULL);
    create_button(s_network_screen, "SCAN", 638, 108, 142, 38, scan_event, NULL);

    s_network_list = lv_obj_create(s_network_screen);
    lv_obj_set_pos(s_network_list, 20, 153);
    lv_obj_set_size(s_network_list, 760, 292);
    lv_obj_set_scrollbar_mode(s_network_list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(s_network_list, lv_color_hex(COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_network_list, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_network_list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_network_list, 0, LV_PART_MAIN);

    s_scan_label = create_label(
        s_network_screen, "Select a network to continue.", 20, 451, 760,
        &lv_font_montserrat_14, COLOR_MUTED);

    s_credentials_screen = lv_obj_create(NULL);
    set_screen_background(s_credentials_screen);
    create_button(s_credentials_screen, "BACK", 12, 12, 86, 38,
                  back_to_networks_event, NULL);
    create_label(s_credentials_screen, "CONNECT TO WI-FI", 116, 17, 430,
                 &lv_font_montserrat_20, COLOR_TEXT);

    create_label(s_credentials_screen, "NETWORK NAME", 20, 79, 120,
                 &lv_font_montserrat_14, COLOR_MUTED);
    s_ssid_field = lv_textarea_create(s_credentials_screen);
    lv_obj_set_pos(s_ssid_field, 145, 70);
    lv_obj_set_size(s_ssid_field, 635, 42);
    lv_obj_set_style_bg_color(s_ssid_field, lv_color_hex(COLOR_CARD), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_ssid_field, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_ssid_field, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
    lv_textarea_set_one_line(s_ssid_field, true);
    lv_textarea_set_max_length(s_ssid_field, APP_WIFI_SSID_MAX_LENGTH);
    lv_textarea_set_placeholder_text(s_ssid_field, "SSID");

    create_label(s_credentials_screen, "PASSWORD", 20, 138, 120,
                 &lv_font_montserrat_14, COLOR_MUTED);
    s_password_field = lv_textarea_create(s_credentials_screen);
    lv_obj_set_pos(s_password_field, 145, 129);
    lv_obj_set_size(s_password_field, 635, 42);
    lv_obj_set_style_bg_color(s_password_field, lv_color_hex(COLOR_CARD), LV_PART_MAIN);
    lv_obj_set_style_text_color(s_password_field, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_password_field, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
    lv_textarea_set_one_line(s_password_field, true);
    lv_textarea_set_password_mode(s_password_field, true);
    lv_textarea_set_max_length(s_password_field, APP_WIFI_PASSWORD_MAX_LENGTH);
    lv_textarea_set_placeholder_text(s_password_field, "Wi-Fi password");

    lv_obj_add_event_cb(s_ssid_field, text_area_event, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_ssid_field, text_area_event, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(s_password_field, text_area_event, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_password_field, text_area_event, LV_EVENT_FOCUSED, NULL);
    create_button(s_credentials_screen, "CONNECT", 638, 183, 142, 38, connect_event, NULL);

    s_credentials_message = create_label(
        s_credentials_screen, "", 20, 188, 590,
        &lv_font_montserrat_14, COLOR_MUTED);
    lv_label_set_long_mode(s_credentials_message, LV_LABEL_LONG_DOT);

    create_keyboard_layout();

    lv_timer_create(settings_timer, 500, NULL);
    return s_network_screen;
}

void app_wifi_settings_show(void)
{
    if (s_network_screen == NULL) {
        ESP_LOGE(TAG, "Wi-Fi settings screen has not been initialized");
        return;
    }

    lv_scr_load(s_network_screen);
    update_connection_label();
    s_scan_rendered = false;
    lv_label_set_text(s_scan_label, "Scanning for networks...");
    esp_err_t err = app_wifi_station_scan();
    if (err == ESP_ERR_NOT_FINISHED) {
        lv_label_set_text(s_scan_label, "A network scan is already in progress...");
    } else if (err != ESP_OK) {
        lv_label_set_text_fmt(s_scan_label, "Scan could not start: %s", esp_err_to_name(err));
        s_scan_rendered = true;
    }
}

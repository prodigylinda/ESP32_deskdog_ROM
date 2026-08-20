/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_err.h"
#include "esp_log.h"
#include "servo_dog_ctrl.h"
#include "nvs_flash.h"

static const char *TAG = "servo_control";

static int s_fl, s_fr, s_bl, s_br = 0;

static int nvs_read_int(const char *key, int default_value)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open("servo_control", NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open nvs");
        return default_value;
    }

    int32_t value;
    if (nvs_get_i32(handle, key, &value) != ESP_OK) {
        return default_value;
    }
    nvs_close(handle);
    return (int)value;
}

static void nvs_write_int(const char *key, int value)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open("servo_control", NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open nvs");
        return;
    }
    if (nvs_set_i32(handle, key, (int32_t)value) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write nvs");
    }
    nvs_commit(handle);
    nvs_close(handle);
}

esp_err_t servo_control_init(void)
{
    s_fl = nvs_read_int("fl", 0);
    s_fr = nvs_read_int("fr", 0);
    s_bl = nvs_read_int("bl", 0);
    s_br = nvs_read_int("br", 0);

    servo_dog_set_leg_offset(s_fl, s_bl, s_fr, s_br);
    return ESP_OK;
}

esp_err_t servo_control_get_save_value(int *fl, int *fr, int *bl, int *br)
{
    s_fl = nvs_read_int("fl", 0);
    s_fr = nvs_read_int("fr", 0);
    s_bl = nvs_read_int("bl", 0);
    s_br = nvs_read_int("br", 0);

    *fl = s_fl;
    *fr = s_fr;
    *bl = s_bl;
    *br = s_br;
    return ESP_OK;
}

esp_err_t servo_control_set_save_value(int fl, int fr, int bl, int br)
{
    s_fl = fl;
    s_fr = fr;
    s_bl = bl;
    s_br = br;
    nvs_write_int("fl", fl);
    nvs_write_int("fr", fr);
    nvs_write_int("bl", bl);
    nvs_write_int("br", br);
    servo_dog_set_leg_offset(fl, bl, fr, br);
    return ESP_OK;
}

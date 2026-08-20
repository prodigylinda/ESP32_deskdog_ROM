/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include "unity.h"

static const char *TAG = "expression_emote_test";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Expression Emote test");
    unity_run_menu();
}

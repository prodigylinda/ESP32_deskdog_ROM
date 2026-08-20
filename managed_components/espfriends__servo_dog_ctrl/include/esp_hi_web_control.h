/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef CONFIG_ESP_HI_WEB_CONTROL_ENABLED

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t esp_hi_web_control_server_init(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_ESP_HI_WEB_CONTROL_ENABLED
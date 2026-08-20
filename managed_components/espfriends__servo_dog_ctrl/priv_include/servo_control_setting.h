/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t servo_control_init(void);

esp_err_t servo_control_get_save_value(int *fl, int *fr, int *bl, int *br);

esp_err_t servo_control_set_save_value(int fl, int fr, int bl, int br);

#ifdef __cplusplus
}
#endif

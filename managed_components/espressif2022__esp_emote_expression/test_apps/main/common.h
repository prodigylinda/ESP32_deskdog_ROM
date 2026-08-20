/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include "expression_emote.h"
#include "gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TEST_APP_ASSETS_PARTITION "anim_icon"

emote_handle_t test_app_emote_create(void);
void test_app_emote_delete(emote_handle_t handle);
void test_app_wait_ms(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

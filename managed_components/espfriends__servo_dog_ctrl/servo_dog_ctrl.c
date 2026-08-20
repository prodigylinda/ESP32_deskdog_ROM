/*
 * SPDX-FileCopyrightText: 2024-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "string.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "iot_servo.h"
#include "servo_dog_ctrl.h"

static const char *TAG = "servo_dog_ctrl";

#define BOW_OFFSET                  50  //Front lying /backward offset angle

// Forward declaration
static void servo_dog_ledc_stop(void);

#define STEP_OFFSET                 5

static uint16_t fl_angle_neutral = CONFIG_FL_ANGLE_NEUTRAL;
static uint16_t fr_angle_neutral = CONFIG_FR_ANGLE_NEUTRAL;
static uint16_t bl_angle_neutral = CONFIG_BL_ANGLE_NEUTRAL;
static uint16_t br_angle_neutral = CONFIG_BR_ANGLE_NEUTRAL;

#define FL_ANGLE_STEP_FORWARD       (fl_angle_neutral - 20)
#define FL_ANGLE_STEP_BACKWARD      (fl_angle_neutral + 20)

#define FR_ANGLE_STEP_FORWARD       (fr_angle_neutral + 20)
#define FR_ANGLE_STEP_BACKWARD      (fr_angle_neutral - 20)

#define BL_ANGLE_STEP_FORWARD       (bl_angle_neutral + 20)
#define BL_ANGLE_STEP_BACKWARD      (bl_angle_neutral - 20)

#define BR_ANGLE_STEP_FORWARD       (br_angle_neutral - 20)
#define BR_ANGLE_STEP_BACKWARD      (br_angle_neutral + 20)

typedef enum {
    SERVO_FL = 0,
    SERVO_FR,
    SERVO_BL,
    SERVO_BR,
} servo_id_t;

typedef void (*servo_dog_action_t)(dog_action_args_t args);

typedef struct {
    servo_dog_action_t funcion;
    dog_action_args_t args;
} servo_dog_action_entry_t;

static const char *g_servo_dog_action_table_name[] = {
    #define X(a, b, c, d, e, f) #a,
    SERVO_DOG_ACTION_TABLE
    #undef X
};

typedef struct {
    servo_dog_state_t state;
    dog_action_args_t args;
} servo_dog_action_msg_t;

typedef struct {
    QueueHandle_t dog_action_queue;
    servo_dog_action_msg_t msg;
    servo_dog_state_t state;
    servo_handle_t servos[4];
    gpio_num_t fl_gpio_num;
    gpio_num_t fr_gpio_num;
    gpio_num_t bl_gpio_num;
    gpio_num_t br_gpio_num;
} servor_dog_ctrl_t;

static servor_dog_ctrl_t *g_servo_dog = NULL;

#define SERVOR_DOG_ACTION_DELAY(delay_ticks) \
    if (xQueuePeek(g_servo_dog->dog_action_queue, &g_servo_dog->msg, delay_ticks) == pdTRUE) { \
        return; \
    } \

void servo_dog_set_leg_offset(int8_t fl_offset, int8_t bl_offset, int8_t fr_offset, int8_t br_offset)
{
    fl_angle_neutral = CONFIG_FL_ANGLE_NEUTRAL + fl_offset;
    fr_angle_neutral = CONFIG_FR_ANGLE_NEUTRAL + fr_offset;
    bl_angle_neutral = CONFIG_BL_ANGLE_NEUTRAL + bl_offset;
    br_angle_neutral = CONFIG_BR_ANGLE_NEUTRAL + br_offset;
}

static void servo_set_angle(servo_id_t servo_id, uint16_t angle)
{
    if (g_servo_dog != NULL && g_servo_dog->servos[servo_id] != NULL) {
        iot_servo_write_angle(g_servo_dog->servos[servo_id], angle);
    }
}

static void servo_dog_neutral(dog_action_args_t args)
{
    servo_set_angle(SERVO_FL, fl_angle_neutral + args.angle_offset);
    servo_set_angle(SERVO_FR, fr_angle_neutral - args.angle_offset);
    servo_set_angle(SERVO_BL, bl_angle_neutral - args.angle_offset);
    servo_set_angle(SERVO_BR, br_angle_neutral + args.angle_offset);
    vTaskDelay(20 / portTICK_PERIOD_MS);
}

static void servo_dog_installation(dog_action_args_t args) {
    // TODO: Implement the installation mode behavior
    servo_set_angle(SERVO_FL, fl_angle_neutral - 70);
    servo_set_angle(SERVO_FR, fr_angle_neutral + 70);
    servo_set_angle(SERVO_BL, bl_angle_neutral + 70);
    servo_set_angle(SERVO_BR, br_angle_neutral - 70);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    servo_dog_ledc_stop();
}

static void servo_dog_forward(dog_action_args_t args)
{
    if (args.speed != 0) {
        uint16_t step_delay_ms = 500 / args.speed;
        for (int step = 0; step < args.repeat_count; step++) {
            for (int i = 0; i < 40; i++) {
                servo_set_angle(SERVO_FL, FL_ANGLE_STEP_BACKWARD - i);
                servo_set_angle(SERVO_BR, BR_ANGLE_STEP_FORWARD + i);
                servo_set_angle(SERVO_BL, BL_ANGLE_STEP_BACKWARD + i - STEP_OFFSET);
                servo_set_angle(SERVO_FR, FR_ANGLE_STEP_FORWARD - i - STEP_OFFSET);

                SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
            }
            SERVOR_DOG_ACTION_DELAY(50 / portTICK_PERIOD_MS);

            for (int i = 0; i < 40; i++) {
                servo_set_angle(SERVO_FL, FL_ANGLE_STEP_FORWARD + i);
                servo_set_angle(SERVO_BR, BR_ANGLE_STEP_BACKWARD - i);
                servo_set_angle(SERVO_BL, BL_ANGLE_STEP_FORWARD - i + STEP_OFFSET);
                servo_set_angle(SERVO_FR, FR_ANGLE_STEP_BACKWARD + i + STEP_OFFSET);
                SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
            }
            SERVOR_DOG_ACTION_DELAY(50 / portTICK_PERIOD_MS);
        }

        for (int i = 0; i < 20; i++) {
            servo_set_angle(SERVO_FL, FL_ANGLE_STEP_BACKWARD - i);
            servo_set_angle(SERVO_BR, BR_ANGLE_STEP_FORWARD + i);
            servo_set_angle(SERVO_BL, BL_ANGLE_STEP_BACKWARD + i);
            servo_set_angle(SERVO_FR, FR_ANGLE_STEP_FORWARD - i);

            SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
        }
    }
}

static void servo_dog_backward(dog_action_args_t args)
{
    if (args.speed != 0) {
        uint16_t step_delay_ms = 500 / args.speed;

        for (int step = 0; step < args.repeat_count; step++) {
            for (int i = 0; i < 40; i++) {
                servo_set_angle(SERVO_BL, BL_ANGLE_STEP_FORWARD - i);
                servo_set_angle(SERVO_FR, FR_ANGLE_STEP_BACKWARD + i);
                servo_set_angle(SERVO_FL, FL_ANGLE_STEP_FORWARD + i - STEP_OFFSET);
                servo_set_angle(SERVO_BR, BR_ANGLE_STEP_BACKWARD - i - STEP_OFFSET);

                SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
            }

            SERVOR_DOG_ACTION_DELAY(50 / portTICK_PERIOD_MS);

            for (int i = 0; i < 40; i++) {
                servo_set_angle(SERVO_BL, BL_ANGLE_STEP_BACKWARD + i);
                servo_set_angle(SERVO_FR, FR_ANGLE_STEP_FORWARD - i);
                servo_set_angle(SERVO_FL, FL_ANGLE_STEP_BACKWARD - i + STEP_OFFSET);
                servo_set_angle(SERVO_BR, BR_ANGLE_STEP_FORWARD + i + STEP_OFFSET);

                SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
            }

            SERVOR_DOG_ACTION_DELAY(50 / portTICK_PERIOD_MS);
        }

        for (int i = 0; i < 20; i++) {
            servo_set_angle(SERVO_BL, BL_ANGLE_STEP_FORWARD - i);
            servo_set_angle(SERVO_FR, FR_ANGLE_STEP_BACKWARD + i);
            servo_set_angle(SERVO_FL, FL_ANGLE_STEP_FORWARD + i);
            servo_set_angle(SERVO_BR, BR_ANGLE_STEP_BACKWARD - i);

            SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
        }
    }
}

static void servo_dog_turn_left(dog_action_args_t args)
{
    if (args.speed != 0) {
        uint16_t step_delay_ms = 500 / args.speed;

        for (int step = 0; step < args.repeat_count; step++) {
            for (int i = 0; i < 40; i++) {
                servo_set_angle(SERVO_FL, FL_ANGLE_STEP_BACKWARD - i + STEP_OFFSET);
                servo_set_angle(SERVO_BR, BR_ANGLE_STEP_BACKWARD - i + STEP_OFFSET);
                servo_set_angle(SERVO_BL, BL_ANGLE_STEP_BACKWARD + i);
                servo_set_angle(SERVO_FR, FR_ANGLE_STEP_BACKWARD + i);

                SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
            }

            for (int i = 0; i < 40; i++) {
                servo_set_angle(SERVO_FL, FL_ANGLE_STEP_FORWARD + i - STEP_OFFSET);
                servo_set_angle(SERVO_BR, BR_ANGLE_STEP_FORWARD + i - STEP_OFFSET);
                servo_set_angle(SERVO_BL, BL_ANGLE_STEP_FORWARD - i);
                servo_set_angle(SERVO_FR, FR_ANGLE_STEP_FORWARD - i);

                SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
            }
        }

        for (int i = 0; i < 20; i++) {
            servo_set_angle(SERVO_FL, FL_ANGLE_STEP_BACKWARD - i);
            servo_set_angle(SERVO_BR, BR_ANGLE_STEP_BACKWARD - i);
            servo_set_angle(SERVO_BL, BL_ANGLE_STEP_BACKWARD + i);
            servo_set_angle(SERVO_FR, FR_ANGLE_STEP_BACKWARD + i);

            SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
        }
    }
}

static void servo_dog_turn_right(dog_action_args_t args)
{
    if (args.speed != 0) {
        uint16_t step_delay_ms = 500 / args.speed;

        for (int step = 0; step < args.repeat_count; step++) {
            for (int i = 0; i < 40; i++) {
                servo_set_angle(SERVO_FL, FL_ANGLE_STEP_FORWARD + i);
                servo_set_angle(SERVO_BR, BR_ANGLE_STEP_FORWARD + i);
                servo_set_angle(SERVO_BL, BL_ANGLE_STEP_FORWARD - i + STEP_OFFSET);
                servo_set_angle(SERVO_FR, FR_ANGLE_STEP_FORWARD - i + STEP_OFFSET);

                SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
            }

            for (int i = 0; i < 40; i++) {
                servo_set_angle(SERVO_FL, FL_ANGLE_STEP_BACKWARD - i - STEP_OFFSET);
                servo_set_angle(SERVO_BR, BR_ANGLE_STEP_BACKWARD - i - STEP_OFFSET);
                servo_set_angle(SERVO_BL, BL_ANGLE_STEP_BACKWARD + i);
                servo_set_angle(SERVO_FR, FR_ANGLE_STEP_BACKWARD + i);
                SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
            }
        }

        for (int i = 0; i < 20; i++) {
            servo_set_angle(SERVO_FL, FL_ANGLE_STEP_FORWARD + i);
            servo_set_angle(SERVO_BR, BR_ANGLE_STEP_FORWARD + i);
            servo_set_angle(SERVO_BL, BL_ANGLE_STEP_FORWARD - i);
            servo_set_angle(SERVO_FR, FR_ANGLE_STEP_FORWARD - i);
            SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
        }
    }
}

static void servo_dog_bow(dog_action_args_t args)
{
    if (args.speed != 0) {
        uint16_t step_delay_ms = 500 / args.speed;

        // From neutrality, gradually forward
        for (int i = 0; i < BOW_OFFSET; i++) {
            servo_set_angle(SERVO_FL, fl_angle_neutral - i);
            servo_set_angle(SERVO_FR, fr_angle_neutral + i);
            servo_set_angle(SERVO_BL, bl_angle_neutral - i);
            servo_set_angle(SERVO_BR, br_angle_neutral + i);
            SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
        }

        // Pause for a while and keep the front lie on your side for a while
        SERVOR_DOG_ACTION_DELAY(args.hold_time_ms / portTICK_PERIOD_MS);

        // Slowly return to neutrality
        for (int i = 0; i < BOW_OFFSET; i++) {
            servo_set_angle(SERVO_FL, fl_angle_neutral - BOW_OFFSET + i);
            servo_set_angle(SERVO_FR, fr_angle_neutral + BOW_OFFSET - i);
            servo_set_angle(SERVO_BL, bl_angle_neutral - BOW_OFFSET + i);
            servo_set_angle(SERVO_BR, br_angle_neutral + BOW_OFFSET - i);
            SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
        }
    }
}

static void servo_dog_lean_back(dog_action_args_t args)
{
    if (args.speed != 0) {
        uint16_t step_delay_ms = 500 / args.speed;
        // From neutral to lean back
        for (int i = 0; i < BOW_OFFSET; i++) {
            servo_set_angle(SERVO_FL, fl_angle_neutral + i);
            servo_set_angle(SERVO_FR, fr_angle_neutral - i);
            servo_set_angle(SERVO_BL, bl_angle_neutral + i);
            servo_set_angle(SERVO_BR, br_angle_neutral - i);
            SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
        }

        // Keep leaning for a while
        SERVOR_DOG_ACTION_DELAY(args.hold_time_ms / portTICK_PERIOD_MS);

        // From the slow fallback movement to the neutral state
        for (int i = 0; i < BOW_OFFSET; i++) {
            servo_set_angle(SERVO_FL, fl_angle_neutral + BOW_OFFSET - i);
            servo_set_angle(SERVO_FR, fr_angle_neutral - BOW_OFFSET + i);
            servo_set_angle(SERVO_BL, bl_angle_neutral + BOW_OFFSET - i);
            servo_set_angle(SERVO_BR, br_angle_neutral - BOW_OFFSET + i);
            SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
        }
    }
}

static void servo_dog_bow_and_lean_back(dog_action_args_t args)
{
    if (args.speed != 0) {
        uint16_t step_delay_ms = 500 / args.speed;

        // Step 1: Middle -> Forward lying (prepare for action)
        for (int i = 0; i < BOW_OFFSET; i++) {
            servo_set_angle(SERVO_FL, fl_angle_neutral - i);
            servo_set_angle(SERVO_FR, fr_angle_neutral + i);
            servo_set_angle(SERVO_BL, bl_angle_neutral - i);
            servo_set_angle(SERVO_BR, br_angle_neutral + i);
            SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
        }

        // Mid-section cycle: forward lying <-> backward
        for (int r = 0; r < args.repeat_count; r++) {
            // Front lying -> Back lying
            for (int i = 0; i < BOW_OFFSET * 2; i++) {
                servo_set_angle(SERVO_FL, fl_angle_neutral - BOW_OFFSET + i);
                servo_set_angle(SERVO_FR, fr_angle_neutral + BOW_OFFSET - i);
                servo_set_angle(SERVO_BL, bl_angle_neutral - BOW_OFFSET + i);
                servo_set_angle(SERVO_BR, br_angle_neutral + BOW_OFFSET - i);
                SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
            }

            // Lean back -> Forward hang
            for (int i = 0; i < BOW_OFFSET * 2; i++) {
                servo_set_angle(SERVO_FL, fl_angle_neutral + BOW_OFFSET - i);
                servo_set_angle(SERVO_FR, fr_angle_neutral - BOW_OFFSET + i);
                servo_set_angle(SERVO_BL, bl_angle_neutral + BOW_OFFSET - i);
                servo_set_angle(SERVO_BR, br_angle_neutral - BOW_OFFSET + i);
                SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
            }
        }

        // The last step: From "forward lying" -> Intermediate state (smooth finishing)
        for (int i = 0; i < BOW_OFFSET; i++) {
            servo_set_angle(SERVO_FL, fl_angle_neutral - BOW_OFFSET + i);
            servo_set_angle(SERVO_FR, fr_angle_neutral + BOW_OFFSET - i);
            servo_set_angle(SERVO_BL, bl_angle_neutral - BOW_OFFSET + i);
            servo_set_angle(SERVO_BR, br_angle_neutral + BOW_OFFSET - i);
            SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
        }
    }
}

static void servo_dog_sway_back_and_forth(dog_action_args_t args)
{
    uint8_t step_delay_ms = 5;
    uint8_t sway_offset = 18;
    for (int i = 0; i < sway_offset; i++) {
        servo_set_angle(SERVO_FL, fl_angle_neutral - i);
        servo_set_angle(SERVO_FR, fr_angle_neutral + i);
        servo_set_angle(SERVO_BL, bl_angle_neutral - i);
        servo_set_angle(SERVO_BR, br_angle_neutral + i);
        SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
    }

    while (sway_offset > 0 && sway_offset <= 18) {
        // Front lying -> Back lying
        for (int i = 0; i < sway_offset * 2; i++) {
            servo_set_angle(SERVO_FL, fl_angle_neutral - sway_offset + i);
            servo_set_angle(SERVO_FR, fr_angle_neutral + sway_offset - i);
            servo_set_angle(SERVO_BL, bl_angle_neutral - sway_offset + i);
            servo_set_angle(SERVO_BR, br_angle_neutral + sway_offset - i);
            SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
        }

        // Lean back -> Forward hang
        for (int i = 0; i < sway_offset * 2; i++) {
            servo_set_angle(SERVO_FL, fl_angle_neutral + sway_offset - i);
            servo_set_angle(SERVO_FR, fr_angle_neutral - sway_offset + i);
            servo_set_angle(SERVO_BL, bl_angle_neutral + sway_offset - i);
            servo_set_angle(SERVO_BR, br_angle_neutral - sway_offset + i);
            SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
        }
        sway_offset -= 3;
    }
}

static void servo_dog_lay_down(dog_action_args_t args)
{
    for (int i = 0; i < 60; i++) {
        servo_set_angle(SERVO_FL, fl_angle_neutral - i);
        servo_set_angle(SERVO_FR, fr_angle_neutral + i);
        servo_set_angle(SERVO_BL, bl_angle_neutral + i);
        servo_set_angle(SERVO_BR, br_angle_neutral - i);
        SERVOR_DOG_ACTION_DELAY(10 / portTICK_PERIOD_MS);
    }
}

static void servo_dog_sway_left_right(dog_action_args_t args)
{
    if (args.speed != 0) {
        uint16_t step_delay_ms = 500 / args.speed;
        dog_action_args_t neutral_arg = {
            .angle_offset = 20,
        };
        servo_dog_neutral(neutral_arg);
        for (int r = 0; r < args.repeat_count; r++) {
            for (int i = 0; i < args.angle_offset; i++) {
                servo_set_angle(SERVO_FL, FL_ANGLE_STEP_BACKWARD - i);
                servo_set_angle(SERVO_FR, FR_ANGLE_STEP_BACKWARD - i);
                servo_set_angle(SERVO_BL, BL_ANGLE_STEP_BACKWARD - i);
                servo_set_angle(SERVO_BR, BR_ANGLE_STEP_BACKWARD - i);
                SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
            }

            for (int i = 0; i < args.angle_offset * 2; i++) {
                servo_set_angle(SERVO_FL, FL_ANGLE_STEP_BACKWARD - args.angle_offset + i);
                servo_set_angle(SERVO_FR, FR_ANGLE_STEP_BACKWARD - args.angle_offset + i);
                servo_set_angle(SERVO_BL, BL_ANGLE_STEP_BACKWARD - args.angle_offset + i);
                servo_set_angle(SERVO_BR, BR_ANGLE_STEP_BACKWARD - args.angle_offset + i);
                SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
            }

            for (int i = 0; i < args.angle_offset; i++) {
                servo_set_angle(SERVO_FL, FL_ANGLE_STEP_BACKWARD + args.angle_offset - i);
                servo_set_angle(SERVO_FR, FR_ANGLE_STEP_BACKWARD + args.angle_offset - i);
                servo_set_angle(SERVO_BL, BL_ANGLE_STEP_BACKWARD + args.angle_offset - i);
                servo_set_angle(SERVO_BR, BR_ANGLE_STEP_BACKWARD + args.angle_offset - i);
                SERVOR_DOG_ACTION_DELAY(step_delay_ms / portTICK_PERIOD_MS);
            }
        }
    }
}

static void servo_dog_shake_hand(dog_action_args_t args)
{
    // Swing the left hind leg back 20 degrees
    for (int i = 0; i < 60; i++) {
        servo_set_angle(SERVO_BL, bl_angle_neutral - i);
        servo_set_angle(SERVO_BR, br_angle_neutral + i);
        SERVOR_DOG_ACTION_DELAY(8 / portTICK_PERIOD_MS);
    }
    // Straighten right front leg
    const int start_angle = fr_angle_neutral + 72;
    const int end_angle = fr_angle_neutral + 57;
    servo_set_angle(SERVO_FR, start_angle);
    // Right front leg swings 15 degrees, handshake
    for (int j = 0; j < args.repeat_count; j++) {
        for (int angle = start_angle; angle >= end_angle; angle--) {
            servo_set_angle(SERVO_FR, angle);
            SERVOR_DOG_ACTION_DELAY(15 / portTICK_PERIOD_MS);
        }
        for (int angle = end_angle; angle <= start_angle; angle++) {
            servo_set_angle(SERVO_FR, angle);
            SERVOR_DOG_ACTION_DELAY(15 / portTICK_PERIOD_MS);
        }
    }
    SERVOR_DOG_ACTION_DELAY(args.hold_time_ms / portTICK_PERIOD_MS);
    // Right front leg back to its original position
    for (int angle = start_angle; angle >= fr_angle_neutral; angle--) {
        servo_set_angle(SERVO_FR, angle);
        SERVOR_DOG_ACTION_DELAY(5 / portTICK_PERIOD_MS);
    }
    // Left hind leg back to its original position
    for (int i = 0; i < 60; i++) {
        servo_set_angle(SERVO_BL, bl_angle_neutral - 60 + i);
        servo_set_angle(SERVO_BR, br_angle_neutral + 60 - i);
        SERVOR_DOG_ACTION_DELAY(8 / portTICK_PERIOD_MS);
    }
}

static void servo_dog_jump_forward(dog_action_args_t args)
{
    servo_dog_neutral(args);
    SERVOR_DOG_ACTION_DELAY(300 / portTICK_PERIOD_MS);
    servo_set_angle(SERVO_FL, FL_ANGLE_STEP_FORWARD - 10);
    servo_set_angle(SERVO_FR, FR_ANGLE_STEP_FORWARD + 10);
    servo_set_angle(SERVO_BL, BL_ANGLE_STEP_BACKWARD - 40);
    servo_set_angle(SERVO_BR, BR_ANGLE_STEP_BACKWARD + 40);
    SERVOR_DOG_ACTION_DELAY(300 / portTICK_PERIOD_MS);
    servo_set_angle(SERVO_FL, FL_ANGLE_STEP_BACKWARD + 50);
    servo_set_angle(SERVO_FR, FR_ANGLE_STEP_BACKWARD - 50);
    SERVOR_DOG_ACTION_DELAY(40 / portTICK_PERIOD_MS);
    servo_set_angle(SERVO_FL, FL_ANGLE_STEP_FORWARD - 50);
    servo_set_angle(SERVO_FR, FR_ANGLE_STEP_FORWARD + 50);
    SERVOR_DOG_ACTION_DELAY(20 / portTICK_PERIOD_MS);
    servo_set_angle(SERVO_BL, BL_ANGLE_STEP_FORWARD);
    servo_set_angle(SERVO_BR, BR_ANGLE_STEP_FORWARD);
    SERVOR_DOG_ACTION_DELAY(150 / portTICK_PERIOD_MS);
    servo_set_angle(SERVO_FL, fl_angle_neutral);
    servo_set_angle(SERVO_FR, fr_angle_neutral);
    SERVOR_DOG_ACTION_DELAY(200 / portTICK_PERIOD_MS);
}

static void servo_dog_jump_backward(dog_action_args_t args)
{
    servo_set_angle(SERVO_FL, FL_ANGLE_STEP_BACKWARD + 20);
    servo_set_angle(SERVO_FR, FR_ANGLE_STEP_BACKWARD - 20);
    servo_set_angle(SERVO_BL, BL_ANGLE_STEP_FORWARD);
    servo_set_angle(SERVO_BR, BR_ANGLE_STEP_FORWARD);
    SERVOR_DOG_ACTION_DELAY(100 / portTICK_PERIOD_MS);
    servo_set_angle(SERVO_FL, FL_ANGLE_STEP_BACKWARD);
    servo_set_angle(SERVO_FR, FR_ANGLE_STEP_BACKWARD);
    servo_set_angle(SERVO_BL, BL_ANGLE_STEP_BACKWARD);
    servo_set_angle(SERVO_BR, BR_ANGLE_STEP_BACKWARD);
    SERVOR_DOG_ACTION_DELAY(100 / portTICK_PERIOD_MS);
    servo_set_angle(SERVO_FL, fl_angle_neutral);
    servo_set_angle(SERVO_FR, fr_angle_neutral);
    servo_set_angle(SERVO_BL, BL_ANGLE_STEP_FORWARD);
    servo_set_angle(SERVO_BR, BR_ANGLE_STEP_FORWARD);
    SERVOR_DOG_ACTION_DELAY(150 / portTICK_PERIOD_MS);
    args.angle_offset = 0;
    servo_dog_neutral(args);
}

static void servo_dog_poke(dog_action_args_t args)
{
    servo_set_angle(SERVO_FL, 0);
    SERVOR_DOG_ACTION_DELAY(20 / portTICK_PERIOD_MS);
    for (int i = 0; i < 5; i++) {
        servo_set_angle(SERVO_FR, fr_angle_neutral + i);
        servo_set_angle(SERVO_BL, bl_angle_neutral - 10 * i);
        servo_set_angle(SERVO_BR, br_angle_neutral + 10 * i);
        SERVOR_DOG_ACTION_DELAY(10 / portTICK_PERIOD_MS);
    }
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 20; i++) {
            servo_set_angle(SERVO_FR, fr_angle_neutral + 5 + i);
            servo_set_angle(SERVO_BL, bl_angle_neutral - 50 - i);
            servo_set_angle(SERVO_BR, br_angle_neutral + 50 + i);
            SERVOR_DOG_ACTION_DELAY(20 / portTICK_PERIOD_MS);
        }
        for (int i = 0; i < 20; i++) {
            servo_set_angle(SERVO_FR, fr_angle_neutral + 25 - i);
            servo_set_angle(SERVO_BL, bl_angle_neutral - 70 + i);
            servo_set_angle(SERVO_BR, br_angle_neutral + 70 - i);
            SERVOR_DOG_ACTION_DELAY(20 / portTICK_PERIOD_MS);
        }
    }
    args.angle_offset = 0;
    servo_dog_neutral(args);
}

static void servo_dog_shake_back_legs(dog_action_args_t args)
{
    for (int i = 0; i < 18; i++) {
        servo_set_angle(SERVO_FL, fl_angle_neutral + 2 * i);
        servo_set_angle(SERVO_FR, fr_angle_neutral - 2 * i);
        servo_set_angle(SERVO_BL, bl_angle_neutral + 3 * i);
        servo_set_angle(SERVO_BR, br_angle_neutral - 3 * i);
        SERVOR_DOG_ACTION_DELAY(15 / portTICK_PERIOD_MS);
    }
    for (int j = 0; j < 12; j++) {
        for (int i = 0; i < 6; i++) {
            servo_set_angle(SERVO_BL, bl_angle_neutral + 54 + i);
            servo_set_angle(SERVO_BR, br_angle_neutral - 54 + i);
            SERVOR_DOG_ACTION_DELAY(7 / portTICK_PERIOD_MS);
        }
        for (int i = 0; i < 12; i++) {
            servo_set_angle(SERVO_BL, bl_angle_neutral + 54 - i);
            servo_set_angle(SERVO_BR, br_angle_neutral - 54 - i);
            SERVOR_DOG_ACTION_DELAY(7 / portTICK_PERIOD_MS);
        }
        for (int i = 0; i < 6; i++) {
            servo_set_angle(SERVO_BL, bl_angle_neutral + 54 + i);
            servo_set_angle(SERVO_BR, br_angle_neutral - 54 + i);
            SERVOR_DOG_ACTION_DELAY(7 / portTICK_PERIOD_MS);
        }
    }
    for (int i = 0; i < 18; i++) {
        servo_set_angle(SERVO_FL, fl_angle_neutral + 36 - 2 * i);
        servo_set_angle(SERVO_FR, fr_angle_neutral - 36 + 2 * i);
        servo_set_angle(SERVO_BL, bl_angle_neutral + 54 - 3 * i);
        servo_set_angle(SERVO_BR, br_angle_neutral - 54 + 3 * i);
        SERVOR_DOG_ACTION_DELAY(15 / portTICK_PERIOD_MS);
    }
}

static void servo_dog_retract_legs(dog_action_args_t args)
{
    for (int i = 0; i < 110; i++) {
        servo_set_angle(SERVO_FL, fl_angle_neutral + i);
        servo_set_angle(SERVO_FR, fr_angle_neutral -i);
        SERVOR_DOG_ACTION_DELAY(4 / portTICK_PERIOD_MS);
    }
    for (int i = 0; i < 103; i++) {
        servo_set_angle(SERVO_BL, bl_angle_neutral - i);
        servo_set_angle(SERVO_BR, br_angle_neutral + i);
        SERVOR_DOG_ACTION_DELAY(4 / portTICK_PERIOD_MS);
    }
}

static void servo_dog_ledc_stop(void)
{
    vTaskDelay(50 / portTICK_PERIOD_MS);
    for (int ch = LEDC_CHANNEL_0; ch <= LEDC_CHANNEL_3; ch++) {
        ledc_stop(LEDC_LOW_SPEED_MODE, ch, 1);
    }

    if (g_servo_dog != NULL) {
        gpio_set_level(g_servo_dog->fl_gpio_num, 1);
        gpio_set_level(g_servo_dog->fr_gpio_num, 1);
        gpio_set_level(g_servo_dog->bl_gpio_num, 1);
        gpio_set_level(g_servo_dog->br_gpio_num, 1);
    }
}

#define X(state, func, repeat, speed, hold, angle_offset) [state] = {.funcion = func, .args = {repeat, speed, hold, angle_offset}},
servo_dog_action_entry_t g_servo_dog_action_table[] = {
    SERVO_DOG_ACTION_TABLE
};
#undef X

static void servo_dog_ctrl_task(void *arg)
{
    dog_action_args_t args = {0};
    // Initialize the server
    servo_dog_neutral(args);

    servo_dog_action_msg_t msg;
    while (1) {
        servo_dog_ledc_stop();
        if (xQueueReceive(g_servo_dog->dog_action_queue, &msg, portMAX_DELAY)) {
            g_servo_dog->state = msg.state;
            ESP_LOGD(TAG, "Servo Dog Action: %s", g_servo_dog_action_table_name[msg.state]);
            if (msg.state < DOG_STATE_MAX) {
                g_servo_dog_action_table[msg.state].funcion(msg.args);
            }
        }
    }
    vTaskDelete(NULL);
    ESP_LOGI(TAG, "Servo Test Task Deleted");
}

static esp_err_t servo_init(servo_dog_ctrl_config_t *config)
{
    ESP_LOGD(TAG, "Servo Control LEDC Channel init");

    const gpio_num_t servo_pins[] = {
        config->fl_gpio_num,
        config->fr_gpio_num,
        config->bl_gpio_num,
        config->br_gpio_num,
    };
    const ledc_channel_t servo_channels[] = {
        LEDC_CHANNEL_0,
        LEDC_CHANNEL_1,
        LEDC_CHANNEL_2,
        LEDC_CHANNEL_3,
    };

    for (int i = 0; i < 4; i++) {
        servo_config_t servo_cfg = SERVO_CONFIG_DEFAULT(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, servo_channels[i], servo_pins[i]);
        ESP_RETURN_ON_ERROR(iot_servo_new(&servo_cfg, &g_servo_dog->servos[i]), TAG, "Failed to create servo %d", i);
    }

    return ESP_OK;
}

esp_err_t servo_dog_ctrl_init(servo_dog_ctrl_config_t *config)
{
    esp_err_t ret = ESP_OK;
    ESP_RETURN_ON_FALSE(g_servo_dog == NULL, ESP_ERR_INVALID_STATE, TAG, "Servo Control already initialized");
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid config");
    g_servo_dog = (servor_dog_ctrl_t *)calloc(1, sizeof(servor_dog_ctrl_t));
    ESP_RETURN_ON_FALSE(g_servo_dog != NULL, ESP_ERR_NO_MEM, TAG, "Failed to allocate memory for servo dog control");
    g_servo_dog->dog_action_queue = xQueueCreate(5, sizeof(servo_dog_action_msg_t));
    ESP_GOTO_ON_FALSE(g_servo_dog->dog_action_queue != NULL, ESP_ERR_NO_MEM, err, TAG, "Failed to create dog action queue");

    g_servo_dog->fl_gpio_num = config->fl_gpio_num;
    g_servo_dog->fr_gpio_num = config->fr_gpio_num;
    g_servo_dog->bl_gpio_num = config->bl_gpio_num;
    g_servo_dog->br_gpio_num = config->br_gpio_num;
    
    // Create servo_dog_ctrl_task
    ESP_GOTO_ON_ERROR(servo_init(config), err, TAG, "Failed to initialize servos");
    xTaskCreate(servo_dog_ctrl_task, "servo_dog_ctrl_task", 2048, NULL, 5, NULL);
    return ESP_OK;
err:
    if (g_servo_dog != NULL) {
        for (int i = 0; i < 4; i++) {
            if (g_servo_dog->servos[i] != NULL) {
                iot_servo_del(g_servo_dog->servos[i]);
            }
        }
    }
    if (g_servo_dog->dog_action_queue != NULL) {
        vQueueDelete(g_servo_dog->dog_action_queue);
    }
    if (g_servo_dog != NULL) {
        free(g_servo_dog);
        g_servo_dog = NULL;
    }
    return ret;
}

esp_err_t servo_dog_ctrl_send(servo_dog_state_t state, dog_action_args_t *args)
{
    if (g_servo_dog == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    servo_dog_action_msg_t msg = {0};
    msg.state = state;

    if (args == NULL) {
        memcpy(&msg.args, &g_servo_dog_action_table[state].args, sizeof(dog_action_args_t));
    } else {
        memcpy(&msg.args, args, sizeof(dog_action_args_t));
    }
    xQueueSend(g_servo_dog->dog_action_queue, &msg, portMAX_DELAY);
    return ESP_OK;
}

/*
 * SPDX-FileCopyrightText: 2024-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int16_t repeat_count;        // -1: not used, Number of times to perform the action
    int16_t speed;               // -1: not used, Speed of execution
    int16_t hold_time_ms;        // -1: not used, Hold duration in milliseconds
    int8_t angle_offset;         // -1: not used, Angle offset for the action
} dog_action_args_t;

#define NOT_USE  -1

#define SERVO_DOG_ACTION_TABLE \
    /* Installation Mode */ \
    X(DOG_STATE_INSTALLATION, servo_dog_installation, /* repeat_count */ NOT_USE, /* speed */ NOT_USE, /* hold_time_ms */ NOT_USE, /* angle_offset */ NOT_USE) \
    /* Idle state, the dog holds its current posture */ \
    X(DOG_STATE_IDLE, servo_dog_neutral, /* repeat_count */ NOT_USE, /* speed */ NOT_USE, /* hold_time_ms */ NOT_USE, /* angle_offset */ 0) \
    /* Move forward, all four legs step forward according to the gait */ \
    X(DOG_STATE_FORWARD, servo_dog_forward, /* repeat_count */ 2, /* speed */ 80, /* hold_time_ms */ NOT_USE, /* angle_offset */ NOT_USE) \
    /* Move backward, all four legs move in reverse */ \
    X(DOG_STATE_BACKWARD, servo_dog_backward, /* repeat_count */ 2, /* speed */ 80, /* hold_time_ms */ NOT_USE, /* angle_offset */ NOT_USE) \
    /* Turn right, coordinated movement of legs to turn right */ \
    X(DOG_STATE_TURN_RIGHT, servo_dog_turn_right, /* repeat_count */ 2, /* speed */ 80, /* hold_time_ms */ NOT_USE, /* angle_offset */ NOT_USE) \
    /* Turn left, coordinated movement of legs to turn left */ \
    X(DOG_STATE_TURN_LEFT, servo_dog_turn_left, /* repeat_count */ 2, /* speed */ 80, /* hold_time_ms */ NOT_USE, /* angle_offset */ NOT_USE) \
    /* Lay down, body lowers closer to the ground to simulate resting */ \
    X(DOG_STATE_LAY_DOWN, servo_dog_lay_down, /* repeat_count */ NOT_USE, /* speed */ NOT_USE, /* hold_time_ms */ NOT_USE, /* angle_offset */ NOT_USE) \
    /* Bowing, front legs press down, hips raised—like a dog stretching or being playful */ \
    X(DOG_STATE_BOW, servo_dog_bow, /* repeat_count */ NOT_USE, /* speed */ 80, /* hold_time_ms */ 500, /* angle_offset */ NOT_USE) \
    /* Lean back, rear legs press down, front legs raised, body leans backward */ \
    X(DOG_STATE_LEAN_BACK, servo_dog_lean_back, /* repeat_count */ NOT_USE, /* speed */ 80, /* hold_time_ms */ 500, /* angle_offset */ NOT_USE) \
    /* A repeated combination of bow and lean back, simulating play or dancing */ \
    X(DOG_STATE_BOW_LEAN, servo_dog_bow_and_lean_back, /* repeat_count */ 2, /* speed */ 80, /* hold_time_ms */ NOT_USE, /* angle_offset */ NOT_USE) \
    /* Sway back and forth */ \
    X(DOG_STATE_SWAY_BACK_FORTH, servo_dog_sway_back_and_forth, /* repeat_count */ NOT_USE, /* speed */ NOT_USE, /* hold_time_ms */ NOT_USE, /* angle_offset */ NOT_USE) \
    /* Sway left and right, body or hips move side to side */ \
    X(DOG_STATE_SWAY, servo_dog_sway_left_right, /* repeat_count */ 2, /* speed */ 40, /* hold_time_ms */ NOT_USE, /* angle_offset */ 20) \
    /* Shake hand action */ \
    X(DOG_STATE_SHAKE_HAND, servo_dog_shake_hand, /* repeat_count */ 10, /* speed */ NOT_USE, /* hold_time_ms */ 3000, /* angle_offset */ NOT_USE) \
    /* Poke action */ \
    X(DOG_STATE_POKE, servo_dog_poke, /* repeat_count */ NOT_USE, /* speed */ NOT_USE, /* hold_time_ms */ NOT_USE, /* angle_offset */ NOT_USE) \
    /* Shake back legs action */ \
    X(DOG_STATE_SHAKE_BACK_LEGS, servo_dog_shake_back_legs, /* repeat_count */ NOT_USE, /* speed */ NOT_USE, /* hold_time_ms */ NOT_USE, /* angle_offset */ 0) \
    /* Jump forward, body jumps forward */ \
    X(DOG_STATE_JUMP_FORWARD, servo_dog_jump_forward, /* repeat_count */ NOT_USE, /* speed */ NOT_USE, /* hold_time_ms */ NOT_USE, /* angle_offset */ NOT_USE) \
    /* Jump backward, body jumps backward */ \
    X(DOG_STATE_JUMP_BACKWARD, servo_dog_jump_backward, /* repeat_count */ NOT_USE, /* speed */ NOT_USE, /* hold_time_ms */ NOT_USE, /* angle_offset */ NOT_USE) \
    /* Retract legs, all four legs move back to the starting position */ \
    X(DOG_STATE_RETRACT_LEGS, servo_dog_retract_legs, /* repeat_count */ NOT_USE, /* speed */ NOT_USE, /* hold_time_ms */ NOT_USE, /* angle_offset */ NOT_USE)

typedef enum {
    #define X(a, b, c, d, e, f) a,
    SERVO_DOG_ACTION_TABLE
    #undef X
    DOG_STATE_MAX,                      // Total number of actions
} servo_dog_state_t;

typedef struct {
    gpio_num_t fl_gpio_num;   // GPIO Number of Front Left Servo
    gpio_num_t fr_gpio_num;   // GPIO Number of Front Right Servo
    gpio_num_t bl_gpio_num;   // GPIO Number of Back Left Servo
    gpio_num_t br_gpio_num;   // GPIO Number of Back Right Servo
} servo_dog_ctrl_config_t;

/**
 * @brief Initialize the servo dog controller
 *
 * @param config Pointer to servo dog controller configuration structure
 *              - fl_gpio_num: GPIO number for front left servo
 *              - fr_gpio_num: GPIO number for front right servo
 *              - bl_gpio_num: GPIO number for back left servo
 *              - br_gpio_num: GPIO number for back right servo
 *
 * @return
 *     - ESP_OK: Successfully initialized servo dog controller
 *     - ESP_ERR_INVALID_STATE: Servo dog controller already initialized
 *     - ESP_ERR_INVALID_ARG: Invalid configuration pointer
 *     - ESP_ERR_NO_MEM: Failed to allocate memory
 */
esp_err_t servo_dog_ctrl_init(servo_dog_ctrl_config_t *config);

/**
 * @brief Send a control command to the servo dog
 *
 * @param state The target state/action for the servo dog to perform
 * @param args Pointer to action arguments structure. If NULL, default arguments will be used
 *
 * @return
 *     - ESP_OK: Successfully sent control command
 *     - ESP_ERR_INVALID_STATE: Servo dog controller not initialized
 */
esp_err_t servo_dog_ctrl_send(servo_dog_state_t state, dog_action_args_t *args);

/*
 * @brief Set the offset of the leg
 * @param fl_offset: The offset of the front left leg
 * @param bl_offset: The offset of the back left leg
 * @param fr_offset: The offset of the front right leg
 * @param br_offset: The offset of the back right leg
 *
 * @note: A positive offset value corrects the leg forward, while a negative offset value corrects it backward.
 *
 * @return: None
 */
void servo_dog_set_leg_offset(int8_t fl_offset, int8_t bl_offset, int8_t fr_offset, int8_t br_offset);

#ifdef __cplusplus
}
#endif

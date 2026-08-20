# Servo Dog Controller

This component provides a set of APIs to control a servo-based robotic dog. It allows the user to perform various actions such as moving forward, backward, turning, and more.

## API Usage

### Initialization

Before using the servo dog controller, you need to initialize it with the configuration parameters.

```c
#include "servo_dog_ctrl.h"

servo_dog_ctrl_config_t config = {
    // Set your configuration parameters here
    .fl_gpio_num = 21,
    .fr_gpio_num = 19,
    .bl_gpio_num = 20,
    .br_gpio_num = 18,
};

esp_err_t err = servo_dog_ctrl_init(&config);
if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize servo dog controller");
}
```

### Sending Commands

You can send commands to the servo dog to perform different actions. For example, to make the dog move forward:

```c
esp_err_t err = servo_dog_ctrl_send(SERVO_DOG_FORWARD, NULL);
if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to send command to servo dog");
}
```

### WebUI

To use the WebUI feature, enable `ESP_HI_WEB_CONTROL_ENABLED` in menuconfig and configure the mDNS hostname using `ESP_HI_MDNS_HOSTNAME`.

```c
#include "servo_dog_ctrl.h"
#include "esp_hi_web_control.h"

// Initialize controller
servo_dog_ctrl_config_t config = {
    // Set your configuration parameters here
    .fl_gpio_num = 21,
    .fr_gpio_num = 19,
    .bl_gpio_num = 20,
    .br_gpio_num = 18,
};

esp_err_t err = servo_dog_ctrl_init(&config);
if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize servo dog controller");
}

// ... after network initialization is complete

// Start WebUI
esp_hi_web_control_server_init();
```

After network initialization is complete, call `esp_hi_web_control_server_init()` to enable remote control via web interface.

## Component URL

For more details, visit the component page: [servo_dog_ctrl](https://components.espressif.com/components/espfriends/servo_dog_ctrl)

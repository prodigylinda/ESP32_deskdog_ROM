set(SRCS "servo_dog_ctrl.c")
set(REQUIRES "esp_driver_gpio" "esp_driver_ledc")
set(EMBED_FILES "")

if(CONFIG_ESP_HI_WEB_CONTROL_ENABLED)
    list(APPEND SRCS "esp_hi_web_control.c" "servo_control_setting.c")
    list(APPEND EMBED_FILES "assets/favicon.ico" "assets/nipplejs.min.js")
    if(CONFIG_LANGUAGE_ZH_CN)
        list(APPEND EMBED_FILES "assets/index.html" "assets/calibration.png" "assets/main.js" "assets/styles.css")
    else()
        list(APPEND EMBED_FILES "assets_en/index.html" "assets_en/calibration.png" "assets_en/main.js" "assets_en/styles.css")
    endif()
endif()

idf_component_register(SRCS ${SRCS}
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "priv_include"
                    REQUIRES ${REQUIRES}
                    PRIV_REQUIRES "esp_http_server" "nvs_flash"
                    EMBED_FILES ${EMBED_FILES})

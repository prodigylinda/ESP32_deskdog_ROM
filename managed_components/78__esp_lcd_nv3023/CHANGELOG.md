# ChangeLog

## v1.0.1 - 2026-07-16

### Added:

* Support for ESP-IDF 6.x series
* Added conditional compilation for RGB element order field (`rgb_ele_order` in IDF 6.0+)
* Added conditional dependency for `esp_driver_gpio` (IDF >= 5.1)

### Modified:

* Updated `reset_gpio_num` type from `int` to `gpio_num_t` for type safety
* Changed GPIO unconnected check from `-1` to `GPIO_NUM_NC` (IDF 6.0+)
* Updated `idf_component.yml` version constraint to `>=4.4`
* Updated `NV3023_PANEL_BUS_SPI_CONFIG` macro to use `GPIO_NUM_NC` instead of `-1`

## v1.0.0 - 2024-08-12

### Enhancements:

* Component version maintenance, code improvement, and documentation enhancement

## v0.0.1 - 2023-12-01

### Enhancements:

* Implement the driver for the NV3022B LCD controller
* Support SPI interface

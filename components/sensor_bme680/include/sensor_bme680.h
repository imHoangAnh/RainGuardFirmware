/**
 * @file sensor_bme680.h
 * @brief Robust BME680 Environmental Sensor Driver with Auto-Detection
 */

#ifndef SENSOR_BME680_H
#define SENSOR_BME680_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BME680 sensor data structure
 */
typedef struct {
    float temperature;    // Temperature in Celsius
    float pressure;       // Pressure in hPa
    float humidity;       // Humidity in %
    float gas_resistance; // Gas resistance in Ohms
} bme680_data_t;

/**
 * @brief Initialize BME680 sensor with auto-address detection
 * 
 * Automatically detects sensor at 0x77 or 0x76, reads calibration data,
 * configures sensor settings and gas heater.
 * 
 * @return ESP_OK on success, ESP_FAIL if sensor not found
 */
esp_err_t sensor_bme680_init(uint8_t i2c_addr)

/**
 * @brief Read sensor data in forced mode
 * 
 * Triggers a forced mode measurement, waits for completion,
 * reads raw ADC values and applies compensation formulas.
 * 
 * @param data Pointer to data structure to fill
 * @return ESP_OK on success
 */
esp_err_t bme680_read_forced(bme680_data_t *data);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_BME680_H

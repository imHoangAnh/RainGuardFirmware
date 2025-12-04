/**
 * @file sensor_bme680.h
 * @brief BME680 Environmental Sensor Driver
 */

#ifndef SENSOR_BME680_H
#define SENSOR_BME680_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// BME680 I2C Address
#define BME680_I2C_ADDR_PRIMARY 0x76
#define BME680_I2C_ADDR_SECONDARY 0x77

    /**
     * @brief BME680 sensor data structure
     */
    typedef struct
    {
        float temperature;    // Temperature in Celsius
        float pressure;       // Pressure in hPa
        float humidity;       // Humidity in %
        float gas_resistance; // Gas resistance in Ohms
    } bme680_data_t;

    /**
     * @brief Initialize BME680 sensor
     * @param i2c_addr I2C address of the sensor
     * @return ESP_OK on success
     */
    esp_err_t sensor_bme680_init(uint8_t i2c_addr);

    /**
     * @brief Read sensor data
     * @param data Pointer to data structure
     * @return ESP_OK on success
     */
    esp_err_t sensor_bme680_read(bme680_data_t *data);

    /**
     * @brief Deinitialize BME680 sensor
     * @return ESP_OK on success
     */
    esp_err_t sensor_bme680_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_BME680_H

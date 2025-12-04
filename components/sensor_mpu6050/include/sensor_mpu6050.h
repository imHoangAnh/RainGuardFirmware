/**
 * @file sensor_mpu6050.h
 * @brief MPU6050 IMU Sensor Driver (Accelerometer + Gyroscope)
 */

#ifndef SENSOR_MPU6050_H
#define SENSOR_MPU6050_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// MPU6050 I2C Address
#define MPU6050_I2C_ADDR_DEFAULT 0x68
#define MPU6050_I2C_ADDR_ALT 0x69

    /**
     * @brief MPU6050 sensor data structure
     */
    typedef struct
    {
        float accel_x; // Acceleration X-axis (g)
        float accel_y; // Acceleration Y-axis (g)
        float accel_z; // Acceleration Z-axis (g)
        float gyro_x;  // Gyroscope X-axis (deg/s)
        float gyro_y;  // Gyroscope Y-axis (deg/s)
        float gyro_z;  // Gyroscope Z-axis (deg/s)
        float temp;    // Temperature (Celsius)
    } mpu6050_data_t;

    /**
     * @brief Initialize MPU6050 sensor
     * @param i2c_addr I2C address of the sensor
     * @return ESP_OK on success
     */
    esp_err_t sensor_mpu6050_init(uint8_t i2c_addr);

    /**
     * @brief Read sensor data
     * @param data Pointer to data structure
     * @return ESP_OK on success
     */
    esp_err_t sensor_mpu6050_read(mpu6050_data_t *data);

    /**
     * @brief Calibrate sensor (zero offsets)
     * @return ESP_OK on success
     */
    esp_err_t sensor_mpu6050_calibrate(void);

    /**
     * @brief Deinitialize MPU6050 sensor
     * @return ESP_OK on success
     */
    esp_err_t sensor_mpu6050_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_MPU6050_H

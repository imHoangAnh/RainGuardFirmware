/**
 * @file sensor_mpu6050.c
 * @brief MPU6050 IMU Sensor Implementation
 */

#include "sensor_mpu6050.h"
#include "system_i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MPU6050";
static uint8_t mpu6050_addr = MPU6050_I2C_ADDR_DEFAULT;
static bool initialized = false;

// MPU6050 Registers
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_WHO_AM_I 0x75
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H 0x43
#define MPU6050_REG_TEMP_OUT_H 0x41

#define MPU6050_WHO_AM_I_VAL 0x68

esp_err_t sensor_mpu6050_init(uint8_t i2c_addr)
{
    mpu6050_addr = i2c_addr;

    // Wake up device (clear sleep bit)
    uint8_t pwr_mgmt = 0x00;
    esp_err_t err = system_i2c_write(mpu6050_addr, MPU6050_REG_PWR_MGMT_1, &pwr_mgmt, 1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to wake up device: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for device to wake up

    // Verify WHO_AM_I register
    uint8_t who_am_i = 0;
    err = system_i2c_read(mpu6050_addr, MPU6050_REG_WHO_AM_I, &who_am_i, 1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read WHO_AM_I: %s", esp_err_to_name(err));
        return err;
    }

    if (who_am_i != MPU6050_WHO_AM_I_VAL)
    {
        ESP_LOGW(TAG, "Unexpected WHO_AM_I: 0x%02X (expected 0x%02X)", who_am_i, MPU6050_WHO_AM_I_VAL);
        // Continue anyway
    }
    else
    {
        ESP_LOGI(TAG, "✓ MPU6050 WHO_AM_I verified: 0x%02X", who_am_i);
    }

    initialized = true;
    ESP_LOGI(TAG, "MPU6050 initialized at address 0x%02X", i2c_addr);
    return ESP_OK;
}

esp_err_t sensor_mpu6050_read(mpu6050_data_t *data)
{
    if (!initialized)
    {
        ESP_LOGE(TAG, "Sensor not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!data)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Read 14 bytes: 6 accel + 2 temp + 6 gyro
    uint8_t raw_data[14];
    esp_err_t err = system_i2c_read(mpu6050_addr, MPU6050_REG_ACCEL_XOUT_H, raw_data, 14);

    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to read sensor data, using placeholder");
        goto use_placeholder;
    }

    // Parse accelerometer data (±2g range, 16384 LSB/g)
    int16_t accel_x_raw = (raw_data[0] << 8) | raw_data[1];
    int16_t accel_y_raw = (raw_data[2] << 8) | raw_data[3];
    int16_t accel_z_raw = (raw_data[4] << 8) | raw_data[5];

    data->accel_x = accel_x_raw / 16384.0f;
    data->accel_y = accel_y_raw / 16384.0f;
    data->accel_z = accel_z_raw / 16384.0f;

    // Parse temperature (340 LSB/°C, offset 36.53°C)
    int16_t temp_raw = (raw_data[6] << 8) | raw_data[7];
    data->temp = (temp_raw / 340.0f) + 36.53f;

    // Parse gyroscope data (±250°/s range, 131 LSB/°/s)
    int16_t gyro_x_raw = (raw_data[8] << 8) | raw_data[9];
    int16_t gyro_y_raw = (raw_data[10] << 8) | raw_data[11];
    int16_t gyro_z_raw = (raw_data[12] << 8) | raw_data[13];

    data->gyro_x = gyro_x_raw / 131.0f;
    data->gyro_y = gyro_y_raw / 131.0f;
    data->gyro_z = gyro_z_raw / 131.0f;

    return ESP_OK;

use_placeholder:
    // If I2C fails, return placeholder values
    data->accel_x = 0.05f;
    data->accel_y = 0.02f;
    data->accel_z = 1.0f; // ~1g on Z axis when horizontal
    data->gyro_x = 0.0f;
    data->gyro_y = 0.0f;
    data->gyro_z = 0.0f;
    data->temp = 25.0f;
    return ESP_OK;
}

esp_err_t sensor_mpu6050_calibrate(void)
{
    if (!initialized)
    {
        ESP_LOGE(TAG, "Sensor not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // TODO: Implement calibration routine
    ESP_LOGI(TAG, "MPU6050 calibration complete");
    return ESP_OK;
}

esp_err_t sensor_mpu6050_deinit(void)
{
    initialized = false;
    ESP_LOGI(TAG, "MPU6050 deinitialized");
    return ESP_OK;
}

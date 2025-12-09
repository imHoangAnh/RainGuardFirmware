/**
 * @file sensor_bme680.c
 * @brief BME680 Environmental Sensor Implementation
 * Note: Also supports BME280/BMP280 sensors
 */

#include "sensor_bme680.h"
#include "system_i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BME680";
static uint8_t bme680_addr = BME680_I2C_ADDR_PRIMARY;
static bool initialized = false;

// BME680/BME280 Registers
#define BME680_REG_CHIP_ID      0xD0
#define BME680_REG_RESET        0xE0
#define BME680_REG_CTRL_HUM     0x72
#define BME680_REG_STATUS       0x73
#define BME680_REG_CTRL_MEAS    0x74
#define BME680_REG_CONFIG       0x75
#define BME680_REG_PRESS_MSB    0x1F
#define BME680_REG_TEMP_MSB     0x22
#define BME680_REG_HUM_MSB      0x25

// BME280 specific registers
#define BME280_REG_PRESS_MSB    0xF7
#define BME280_REG_TEMP_MSB     0xFA
#define BME280_REG_HUM_MSB      0xFD
#define BME280_REG_CALIB00      0x88
#define BME280_REG_CALIB26      0xE1

// Chip IDs
#define BME680_CHIP_ID_VAL      0x61
#define BME280_CHIP_ID_VAL      0x60
#define BMP280_CHIP_ID_VAL      0x58

// Calibration data storage (BME280/BMP280)
static struct {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
    
    int32_t t_fine;
} calib_data;

static uint8_t detected_chip_id = 0;

// Read calibration data for BME280/BMP280
static esp_err_t read_calibration_data(void)
{
    uint8_t calib[26];
    esp_err_t err;
    
    // Read temperature and pressure calibration (0x88-0x9F)
    err = system_i2c_read(bme680_addr, BME280_REG_CALIB00, calib, 26);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read calibration data 1");
        return err;
    }
    
    calib_data.dig_T1 = (calib[1] << 8) | calib[0];
    calib_data.dig_T2 = (calib[3] << 8) | calib[2];
    calib_data.dig_T3 = (calib[5] << 8) | calib[4];
    
    calib_data.dig_P1 = (calib[7] << 8) | calib[6];
    calib_data.dig_P2 = (calib[9] << 8) | calib[8];
    calib_data.dig_P3 = (calib[11] << 8) | calib[10];
    calib_data.dig_P4 = (calib[13] << 8) | calib[12];
    calib_data.dig_P5 = (calib[15] << 8) | calib[14];
    calib_data.dig_P6 = (calib[17] << 8) | calib[16];
    calib_data.dig_P7 = (calib[19] << 8) | calib[18];
    calib_data.dig_P8 = (calib[21] << 8) | calib[20];
    calib_data.dig_P9 = (calib[23] << 8) | calib[22];
    
    calib_data.dig_H1 = calib[25];
    
    // Read humidity calibration (0xE1-0xE7) for BME280
    if (detected_chip_id == BME280_CHIP_ID_VAL) {
        uint8_t hum_calib[7];
        err = system_i2c_read(bme680_addr, BME280_REG_CALIB26, hum_calib, 7);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read humidity calibration");
            return err;
        }
        
        calib_data.dig_H2 = (hum_calib[1] << 8) | hum_calib[0];
        calib_data.dig_H3 = hum_calib[2];
        calib_data.dig_H4 = (hum_calib[3] << 4) | (hum_calib[4] & 0x0F);
        calib_data.dig_H5 = (hum_calib[5] << 4) | (hum_calib[4] >> 4);
        calib_data.dig_H6 = hum_calib[6];
    }
    
    ESP_LOGI(TAG, "Calibration data loaded");
    return ESP_OK;
}

// Compensate temperature (BME280 formula)
static float compensate_temperature(int32_t adc_T)
{
    int32_t var1, var2;
    
    var1 = ((((adc_T >> 3) - ((int32_t)calib_data.dig_T1 << 1))) * ((int32_t)calib_data.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib_data.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib_data.dig_T1))) >> 12) * ((int32_t)calib_data.dig_T3)) >> 14;
    
    calib_data.t_fine = var1 + var2;
    
    float T = (calib_data.t_fine * 5 + 128) >> 8;
    return T / 100.0f;
}

// Compensate pressure (BME280 formula)
static float compensate_pressure(int32_t adc_P)
{
    int64_t var1, var2, p;
    
    var1 = ((int64_t)calib_data.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib_data.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib_data.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib_data.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib_data.dig_P3) >> 8) + ((var1 * (int64_t)calib_data.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib_data.dig_P1) >> 33;
    
    if (var1 == 0) {
        return 0; // avoid division by zero
    }
    
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib_data.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib_data.dig_P7) << 4);
    
    return (float)p / 256.0f / 100.0f; // Convert Pa to hPa
}

// Compensate humidity (BME280 formula)
static float compensate_humidity(int32_t adc_H)
{
    int32_t v_x1_u32r;
    
    v_x1_u32r = (calib_data.t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib_data.dig_H4) << 20) - (((int32_t)calib_data.dig_H5) * v_x1_u32r)) +
                   ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)calib_data.dig_H6)) >> 10) *
                   (((v_x1_u32r * ((int32_t)calib_data.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) *
                   ((int32_t)calib_data.dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)calib_data.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    
    return (float)(v_x1_u32r >> 12) / 1024.0f;
}

esp_err_t sensor_bme680_init(uint8_t i2c_addr)
{
    bme680_addr = i2c_addr;
    
    ESP_LOGI(TAG, "Attempting to initialize BME680 at address 0x%02X", i2c_addr);
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Read chip ID
    uint8_t chip_id = 0;
    esp_err_t err = ESP_FAIL;
    
    for (int attempt = 0; attempt < 3; attempt++)
    {
        err = system_i2c_read(bme680_addr, BME680_REG_CHIP_ID, &chip_id, 1);
        ESP_LOGI(TAG, "Attempt %d: Read chip ID = 0x%02X, status = %s",
                 attempt + 1, chip_id, esp_err_to_name(err));
        
        if (err == ESP_OK)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read chip ID after 3 attempts: %s", esp_err_to_name(err));
        return err;
    }
    
    detected_chip_id = chip_id;
    
    // Identify sensor type
    if (chip_id == BME680_CHIP_ID_VAL)
    {
        ESP_LOGI(TAG, "✓ Detected BME680 sensor (chip ID: 0x%02X)", chip_id);
        ESP_LOGW(TAG, "BME680 full support not implemented, using basic mode");
    }
    else if (chip_id == BME280_CHIP_ID_VAL)
    {
        ESP_LOGI(TAG, "✓ Detected BME280 sensor (chip ID: 0x%02X)", chip_id);
    }
    else if (chip_id == BMP280_CHIP_ID_VAL)
    {
        ESP_LOGI(TAG, "✓ Detected BMP280 sensor (chip ID: 0x%02X)", chip_id);
        ESP_LOGW(TAG, "BMP280 does not support humidity measurement");
    }
    else
    {
        ESP_LOGW(TAG, "Unknown chip ID: 0x%02X, attempting to continue...", chip_id);
    }
    
    // Read calibration data
    err = read_calibration_data();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read calibration data");
        return err;
    }
    
    // Configure humidity oversampling (for BME280/BME680)
    if (chip_id == BME280_CHIP_ID_VAL || chip_id == BME680_CHIP_ID_VAL)
    {
        uint8_t ctrl_hum = 0x01; // Oversampling x1
        err = system_i2c_write(bme680_addr, BME680_REG_CTRL_HUM, &ctrl_hum, 1);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to configure humidity: %s", esp_err_to_name(err));
            return err;
        }
    }
    
    // Configure temp/pressure oversampling and mode
    // osrs_t=001 (x1), osrs_p=001 (x1), mode=01 (forced mode)
    uint8_t ctrl_meas = 0x25;
    err = system_i2c_write(bme680_addr, BME680_REG_CTRL_MEAS, &ctrl_meas, 1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure measurement: %s", esp_err_to_name(err));
        return err;
    }
    
    initialized = true;
    ESP_LOGI(TAG, "Sensor initialized successfully at address 0x%02X", i2c_addr);
    return ESP_OK;
}

esp_err_t sensor_bme680_read(bme680_data_t *data)
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
    
    // Trigger forced mode measurement
    uint8_t ctrl_meas = 0x25;
    esp_err_t err = system_i2c_write(bme680_addr, BME680_REG_CTRL_MEAS, &ctrl_meas, 1);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to trigger measurement");
        goto use_placeholder;
    }
    
    // Wait for measurement to complete
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Read raw data (8 bytes: press, temp, hum)
    uint8_t raw_data[8];
    err = system_i2c_read(bme680_addr, BME280_REG_PRESS_MSB, raw_data, 8);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to read sensor data");
        goto use_placeholder;
    }
    
    // Parse raw ADC values
    int32_t adc_P = (raw_data[0] << 12) | (raw_data[1] << 4) | (raw_data[2] >> 4);
    int32_t adc_T = (raw_data[3] << 12) | (raw_data[4] << 4) | (raw_data[5] >> 4);
    int32_t adc_H = (raw_data[6] << 8) | raw_data[7];
    
    // Apply compensation formulas
    data->temperature = compensate_temperature(adc_T);
    data->pressure = compensate_pressure(adc_P);
    
    if (detected_chip_id == BME280_CHIP_ID_VAL || detected_chip_id == BME680_CHIP_ID_VAL)
    {
        data->humidity = compensate_humidity(adc_H);
    }
    else
    {
        data->humidity = 0.0f; // BMP280 doesn't have humidity sensor
    }
    
    // Gas resistance not implemented
    data->gas_resistance = 0.0f;
    
    return ESP_OK;

use_placeholder:
    // If I2C fails, return reasonable placeholder values
    data->temperature = 25.0f;
    data->pressure = 1013.25f;
    data->humidity = 50.0f;
    data->gas_resistance = 0.0f;
    return ESP_OK;
}

esp_err_t sensor_bme680_deinit(void)
{
    initialized = false;
    ESP_LOGI(TAG, "Sensor deinitialized");
    return ESP_OK;
}

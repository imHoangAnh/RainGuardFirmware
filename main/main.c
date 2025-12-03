/**
 * @file main.c
 * @brief RainGuard Main Application - Full Sensor MQTT Test
 *
 * Target: ESP32-S3-WROOM N16R8 (16MB Flash, 8MB PSRAM)
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_psram.h"
#include "driver/uart.h"

// Component headers
#include "cam_config.h"
#include "pin_config.h"
#include "app_network.h"
#include "system_i2c.h"
#include "sensor_bme680.h"
#include "sensor_mpu6050.h"
#include "gps_neo6m.h"

static const char *TAG = "MAIN";

// ============================================================================
// Configuration
// ============================================================================
#define DEVICE_ID "ESP32_Train_01"
#define MQTT_BROKER_URI "mqtt://192.168.0.102:1883" // Change to your PC IP
#define MQTT_TOPIC "train/data/" DEVICE_ID
#define SENSOR_READ_INTERVAL_MS 5000 // 5 seconds

// ============================================================================
// Sensor Data Collection Task
// ============================================================================
static void sensor_mqtt_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor MQTT task started");
    char json_buffer[512];

    while (1)
    {
        // Read BME680 (Temperature, Humidity, Gas)
        bme680_data_t bme_data = {0};
        bme680_read_forced(&bme_data);

        // Read MPU6050 (Accelerometer, Gyroscope)
        mpu6050_data_t mpu_data = {0};
        sensor_mpu6050_read(&mpu_data);

        // Read GPS (Location, Speed)
        gps_data_t gps_data = {0};
        gps_neo6m_read(&gps_data, 1000); // 1 second timeout

        // Calculate vibration magnitude from accelerometer
        float vibration = sqrtf(mpu_data.accel_x * mpu_data.accel_x +
                                mpu_data.accel_y * mpu_data.accel_y +
                                mpu_data.accel_z * mpu_data.accel_z) -
                          1.0f;
        if (vibration < 0)
            vibration = 0;

        // Format JSON payload
        snprintf(json_buffer, sizeof(json_buffer),
                 "{"
                 "\"deviceId\":\"%s\","
                 "\"temp\":%.2f,"
                 "\"hum\":%.2f,"
                 "\"pressure\":%.2f,"
                 "\"gas\":%.0f,"
                 "\"lat\":%.6f,"
                 "\"lng\":%.6f,"
                 "\"speed\":%.2f,"
                 "\"vibration\":%.3f,"
                 "\"accel_x\":%.3f,"
                 "\"accel_y\":%.3f,"
                 "\"accel_z\":%.3f"
                 "}",
                 DEVICE_ID,
                 bme_data.temperature,
                 bme_data.humidity,
                 bme_data.pressure,
                 bme_data.gas_resistance,
                 gps_data.latitude,
                 gps_data.longitude,
                 gps_data.speed,
                 vibration,
                 mpu_data.accel_x,
                 mpu_data.accel_y,
                 mpu_data.accel_z);

        // Log to console
        ESP_LOGI(TAG, "📊 Sensor Data: %s", json_buffer);

        // Publish to MQTT
        if (app_network_mqtt_is_connected())
        {
            esp_err_t err = app_network_mqtt_publish(MQTT_TOPIC, json_buffer, 0);
            if (err == ESP_OK)
            {
                ESP_LOGI(TAG, "✓ Published to MQTT topic: %s", MQTT_TOPIC);
            }
        }
        else
        {
            ESP_LOGW(TAG, "MQTT not connected, message not sent");
        }

        // Wait for next interval
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

// ============================================================================
// Initialize NVS
// ============================================================================
static esp_err_t init_nvs(void)
{
    ESP_LOGI(TAG, "Initializing NVS...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS partition needs to be erased, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "✓ NVS initialized successfully");
    }
    else
    {
        ESP_LOGE(TAG, "✗ NVS initialization failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

// ============================================================================
// Main Application Entry Point
// ============================================================================
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  RainGuard - ESP32-S3 N16R8");
    ESP_LOGI(TAG, "========================================");

    // Print system information
    ESP_LOGI(TAG, "ESP-IDF Version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "PSRAM size: %lu bytes", esp_psram_get_size());

    // Step 1: Initialize NVS
    ESP_ERROR_CHECK(init_nvs());

    // Step 2: Initialize WiFi and wait for connection
    ESP_LOGI(TAG, "Initializing WiFi...");
    ESP_ERROR_CHECK(app_network_init());

    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    if (!app_network_wait_connected(300))
    {
        ESP_LOGE(TAG, "Failed to connect to WiFi, cannot continue");
        return;
    }

    char ip_str[16];
    app_network_get_ip(ip_str);
    ESP_LOGI(TAG, "✓ WiFi Connected, IP: %s", ip_str);

    // Step 3: Initialize MQTT
    ESP_LOGI(TAG, "Initializing MQTT client...");
    ESP_LOGI(TAG, "Broker URI: %s", MQTT_BROKER_URI);
    ESP_ERROR_CHECK(app_network_mqtt_init(MQTT_BROKER_URI));

    // Wait for MQTT connection
    for (int i = 0; i < 10; i++)
    {
        if (app_network_mqtt_is_connected())
        {
            ESP_LOGI(TAG, "✓ MQTT Connected to broker");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    // Step 4: Initialize I2C bus
    ESP_LOGI(TAG, "Initializing I2C bus...");
    ESP_ERROR_CHECK(system_i2c_init(I2C_SDA_PIN, I2C_SCL_PIN));
    ESP_LOGI(TAG, "✓ I2C bus initialized (SDA:%d, SCL:%d)", I2C_SDA_PIN, I2C_SCL_PIN);

    // Step 5: Initialize Sensors
    ESP_LOGI(TAG, "Initializing sensors...");

    // BME680 (Auto-detects address)
    esp_err_t err = sensor_bme680_init(BME680_I2C_ADDR_DEFAULT);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "✓ BME680 initialized");
    }
    else
    {
        ESP_LOGW(TAG, "⚠ BME680 init failed, will use placeholder data");
    }

    // MPU6050
    err = sensor_mpu6050_init(MPU6050_I2C_ADDR_DEFAULT);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "✓ MPU6050 initialized");
    }
    else
    {
        ESP_LOGW(TAG, "⚠ MPU6050 init failed, will use placeholder data");
    }

    // GPS NEO-6M
    err = gps_neo6m_init(GPS_UART_NUM, GPS_UART_TX, GPS_UART_RX, GPS_BAUD_RATE);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "✓ GPS initialized (TX:%d, RX:%d)", GPS_UART_TX, GPS_UART_RX);
    }
    else
    {
        ESP_LOGW(TAG, "⚠ GPS init failed, will use placeholder data");
    }

    // Step 6: Start Sensor MQTT Task
    ESP_LOGI(TAG, "Starting sensor MQTT task (interval: %d ms)...", SENSOR_READ_INTERVAL_MS);
    xTaskCreatePinnedToCore(
        sensor_mqtt_task, // Task function
        "sensor_mqtt",    // Task name
        8192,             // Stack size (8KB)
        NULL,             // Parameters
        5,                // Priority
        NULL,             // Task handle
        1                 // Core ID (1 = APP_CPU)
    );

    ESP_LOGI(TAG, "✓ System initialization complete");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Publishing sensor data to topic: %s", MQTT_TOPIC);
    ESP_LOGI(TAG, "========================================");

    // Main loop - monitor system health
    uint32_t loop_count = 0;
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(60)); // Every 60 seconds
        loop_count++;

        ESP_LOGI(TAG, "System Status [Uptime: %lu min]", loop_count);
        ESP_LOGI(TAG, "  Free heap: %lu bytes", esp_get_free_heap_size());
        ESP_LOGI(TAG, "  Min free heap: %lu bytes", esp_get_minimum_free_heap_size());
        ESP_LOGI(TAG, "  WiFi: %s, MQTT: %s",
                 app_network_get_status() == NETWORK_CONNECTED ? "Connected" : "Disconnected",
                 app_network_mqtt_is_connected() ? "Connected" : "Disconnected");
    }
}

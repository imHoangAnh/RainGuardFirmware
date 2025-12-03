/**
 * @file app_network.h
 * @brief WiFi, MQTT, and HTTP Client Management
 */

#ifndef APP_NETWORK_H
#define APP_NETWORK_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network connection status
 */
typedef enum {
    NETWORK_DISCONNECTED = 0,
    NETWORK_CONNECTING,
    NETWORK_CONNECTED,
    NETWORK_ERROR
} network_status_t;

/**
 * @brief Initialize network subsystem and connect to WiFi
 * Uses hardcoded credentials for testing
 * @return ESP_OK on success
 */
esp_err_t app_network_init(void);

/**
 * @brief Get current network status
 * @return Current network status
 */
network_status_t app_network_get_status(void);

/**
 * @brief Wait for WiFi connection
 * @param timeout_ms Timeout in milliseconds
 * @return true if connected, false if timeout
 */
bool app_network_wait_connected(uint32_t timeout_ms);

/**
 * @brief Get IP address as string
 * @param ip_str Buffer to store IP string (min 16 bytes)
 * @return ESP_OK on success
 */
esp_err_t app_network_get_ip(char *ip_str);

// ============================================================================
// MQTT Functions
// ============================================================================

/**
 * @brief Initialize MQTT client and connect to broker
 * @param broker_uri MQTT broker URI (e.g., "mqtt://192.168.1.100:1883")
 * @return ESP_OK on success
 */
esp_err_t app_network_mqtt_init(const char *broker_uri);

/**
 * @brief Publish data to MQTT topic
 * @param topic Topic string
 * @param data Data payload (JSON string or binary)
 * @param len Length of data (0 = use strlen)
 * @return ESP_OK on success
 */
esp_err_t app_network_mqtt_publish(const char *topic, const char *data, size_t len);

/**
 * @brief Check if MQTT client is connected
 * @return true if connected
 */
bool app_network_mqtt_is_connected(void);

// ============================================================================
// HTTP Functions (Legacy - for HTTP upload)
// ============================================================================

/**
 * @brief Upload image data via HTTP POST
 * @param url Target URL
 * @param image_data Pointer to image data
 * @param image_size Size of image data
 * @return ESP_OK on success
 */
esp_err_t app_network_upload_image(const char *url, const uint8_t *image_data, size_t image_size);

/**
 * @brief Upload JSON data via HTTP POST
 * @param url Target URL
 * @param json_data Pointer to JSON string
 * @return ESP_OK on success
 */
esp_err_t app_network_upload_json(const char *url, const char *json_data);

#ifdef __cplusplus
}
#endif

#endif // APP_NETWORK_H


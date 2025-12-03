/**
 * @file app_network.c
 * @brief WiFi, MQTT, and HTTP Client Implementation
 */

#include "app_network.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "APP_NETWORK";

// ============================================================================
// WiFi Configuration (Hardcoded for testing)
// ============================================================================
#define WIFI_SSID "TP-Link_FAFC"
#define WIFI_PASS "29504923"
#define WIFI_MAX_RETRY 10

// ============================================================================
// State Variables
// ============================================================================
static network_status_t current_status = NETWORK_DISCONNECTED;
static esp_netif_t *sta_netif = NULL;
static int wifi_retry_count = 0;
static EventGroupHandle_t wifi_event_group;
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// ============================================================================
// WiFi Event Handler
// ============================================================================
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        current_status = NETWORK_CONNECTING;
        ESP_LOGI(TAG, "WiFi started, connecting...");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (wifi_retry_count < WIFI_MAX_RETRY)
        {
            esp_wifi_connect();
            wifi_retry_count++;
            ESP_LOGI(TAG, "Retry to connect to the AP (%d/%d)", wifi_retry_count, WIFI_MAX_RETRY);
            current_status = NETWORK_CONNECTING;
        }
        else
        {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Failed to connect to WiFi");
            current_status = NETWORK_ERROR;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "✓ WiFi Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_retry_count = 0;
        current_status = NETWORK_CONNECTED;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// ============================================================================
// MQTT Event Handler
// ============================================================================
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "✓ MQTT Connected to broker");
        mqtt_connected = true;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT Disconnected");
        mqtt_connected = false;
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "MQTT message published, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT Error");
        break;
    default:
        break;
    }
}

// ============================================================================
// Public Functions
// ============================================================================

esp_err_t app_network_init(void)
{
    ESP_LOGI(TAG, "Initializing WiFi...");

    // Create event group
    wifi_event_group = xEventGroupCreate();

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    sta_netif = esp_netif_create_default_wifi_sta();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // Configure WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi init complete, connecting to SSID: %s", WIFI_SSID);

    return ESP_OK;
}

network_status_t app_network_get_status(void)
{
    return current_status;
}

bool app_network_wait_connected(uint32_t timeout_ms)
{
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(timeout_ms));

    if (bits & WIFI_CONNECTED_BIT)
    {
        return true;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
        return false;
    }
    else
    {
        ESP_LOGE(TAG, "WiFi connection timeout");
        return false;
    }
}

esp_err_t app_network_get_ip(char *ip_str)
{
    if (!ip_str)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (current_status != NETWORK_CONNECTED || !sta_netif)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(sta_netif, &ip_info);
    sprintf(ip_str, IPSTR, IP2STR(&ip_info.ip));

    return ESP_OK;
}

// ============================================================================
// MQTT Functions
// ============================================================================

esp_err_t app_network_mqtt_init(const char *broker_uri)
{
    if (!broker_uri)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (current_status != NETWORK_CONNECTED)
    {
        ESP_LOGE(TAG, "WiFi not connected, cannot init MQTT");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Initializing MQTT client...");
    ESP_LOGI(TAG, "Broker URI: %s", broker_uri);

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
        .session.keepalive = 60,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!mqtt_client)
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));

    ESP_LOGI(TAG, "MQTT client started");
    return ESP_OK;
}

esp_err_t app_network_mqtt_publish(const char *topic, const char *data, size_t len)
{
    if (!mqtt_client)
    {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!mqtt_connected)
    {
        ESP_LOGW(TAG, "MQTT not connected, message dropped");
        return ESP_ERR_INVALID_STATE;
    }

    if (!topic || !data)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // If len is 0, calculate string length
    if (len == 0)
    {
        len = strlen(data);
    }

    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, data, len, 1, 0);
    if (msg_id < 0)
    {
        ESP_LOGE(TAG, "Failed to publish MQTT message");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Published to topic '%s', msg_id=%d, len=%d", topic, msg_id, len);
    return ESP_OK;
}

bool app_network_mqtt_is_connected(void)
{
    return mqtt_connected;
}

// ============================================================================
// HTTP Functions (Legacy)
// ============================================================================

esp_err_t app_network_upload_image(const char *url, const uint8_t *image_data, size_t image_size)
{
    if (!url || !image_data || image_size == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (current_status != NETWORK_CONNECTED)
    {
        ESP_LOGE(TAG, "Not connected to network");
        return ESP_ERR_INVALID_STATE;
    }

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_post_field(client, (const char *)image_data, image_size);

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Image uploaded, status=%d, size=%zu bytes", status, image_size);
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

esp_err_t app_network_upload_json(const char *url, const char *json_data)
{
    if (!url || !json_data)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (current_status != NETWORK_CONNECTED)
    {
        ESP_LOGE(TAG, "Not connected to network");
        return ESP_ERR_INVALID_STATE;
    }

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "JSON uploaded, status=%d", status);
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

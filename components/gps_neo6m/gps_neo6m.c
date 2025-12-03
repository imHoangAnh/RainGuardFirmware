/**
 * @file gps_neo6m.c
 * @brief NEO-6M GPS Module Implementation
 */

#include "gps_neo6m.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "GPS_NEO6M";
static int gps_uart_num = -1;
static bool initialized = false;
static gps_data_t last_valid_data = {0};

#define GPS_UART_BUF_SIZE (1024)
#define NMEA_MAX_LENGTH (128)

// Simple NMEA parser helper
static bool parse_gprmc(const char *sentence, gps_data_t *data)
{
    // $GPRMC format: $GPRMC,time,status,lat,N/S,lon,E/W,speed,course,date,mag,E/W,mode*checksum
    char temp[128];
    strncpy(temp, sentence, sizeof(temp) - 1);

    char *token = strtok(temp, ",");
    int field = 0;
    char lat_str[16] = {0}, lon_str[16] = {0};
    char lat_dir = 'N', lon_dir = 'E';
    char status = 'V';

    while (token != NULL && field < 12)
    {
        switch (field)
        {
        case 2:
            status = token[0];
            break; // A=valid, V=invalid
        case 3:
            strncpy(lat_str, token, sizeof(lat_str) - 1);
            break;
        case 4:
            lat_dir = token[0];
            break;
        case 5:
            strncpy(lon_str, token, sizeof(lon_str) - 1);
            break;
        case 6:
            lon_dir = token[0];
            break;
        case 7:
            data->speed = atof(token) * 1.852f;
            break; // knots to km/h
        case 8:
            data->course = atof(token);
            break;
        }
        token = strtok(NULL, ",");
        field++;
    }

    if (status != 'A')
    {
        return false; // No GPS fix
    }

    // Convert DDMM.MMMM to decimal degrees
    if (strlen(lat_str) > 0)
    {
        float lat_deg = atof(lat_str) / 100.0f;
        int lat_d = (int)lat_deg;
        float lat_m = (lat_deg - lat_d) * 100.0f / 60.0f;
        data->latitude = lat_d + lat_m;
        if (lat_dir == 'S')
            data->latitude = -data->latitude;
    }

    if (strlen(lon_str) > 0)
    {
        float lon_deg = atof(lon_str) / 100.0f;
        int lon_d = (int)lon_deg;
        float lon_m = (lon_deg - lon_d) * 100.0f / 60.0f;
        data->longitude = lon_d + lon_m;
        if (lon_dir == 'W')
            data->longitude = -data->longitude;
    }

    data->valid = true;
    return true;
}

esp_err_t gps_neo6m_init(int uart_num, int tx_pin, int rx_pin, int baud_rate)
{
    gps_uart_num = uart_num;

    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_param_config(uart_num, &uart_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "UART param config failed");
        return err;
    }

    err = uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "UART set pin failed");
        return err;
    }

    err = uart_driver_install(uart_num, GPS_UART_BUF_SIZE, GPS_UART_BUF_SIZE, 0, NULL, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "UART driver install failed");
        return err;
    }

    initialized = true;
    ESP_LOGI(TAG, "GPS initialized (UART%d, TX:%d, RX:%d, Baud:%d)", uart_num, tx_pin, rx_pin, baud_rate);
    return ESP_OK;
}

esp_err_t gps_neo6m_read(gps_data_t *data, uint32_t timeout_ms)
{
    if (!initialized)
    {
        ESP_LOGE(TAG, "GPS not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!data)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Try to read NMEA sentences
    uint8_t line[NMEA_MAX_LENGTH];
    int line_pos = 0;
    bool found_rmc = false;

    int64_t start_time = esp_timer_get_time() / 1000; // Convert to ms

    while ((esp_timer_get_time() / 1000 - start_time) < timeout_ms)
    {
        uint8_t c;
        int len = uart_read_bytes(gps_uart_num, &c, 1, pdMS_TO_TICKS(100));

        if (len > 0)
        {
            if (c == '\n')
            {
                line[line_pos] = '\0';

                // Check if this is a GPRMC sentence
                if (strncmp((char *)line, "$GPRMC", 6) == 0)
                {
                    if (parse_gprmc((char *)line, data))
                    {
                        last_valid_data = *data;
                        found_rmc = true;
                        ESP_LOGI(TAG, "GPS Fix: lat=%.6f, lon=%.6f, speed=%.1f km/h",
                                 data->latitude, data->longitude, data->speed);
                        return ESP_OK;
                    }
                }

                line_pos = 0;
            }
            else if (line_pos < NMEA_MAX_LENGTH - 1)
            {
                line[line_pos++] = c;
            }
        }
    }

    // Timeout or no fix - return placeholder data
    ESP_LOGW(TAG, "GPS no fix, using placeholder data");
    data->valid = false;
    data->latitude = 21.028511; // Hanoi coordinates (placeholder)
    data->longitude = 105.804817;
    data->altitude = 10.0f;
    data->speed = 0.0f;
    data->satellites = 0;

    return ESP_OK; // Return OK so MQTT flow continues
}

esp_err_t gps_neo6m_get_last_data(gps_data_t *data)
{
    if (!initialized)
    {
        ESP_LOGE(TAG, "GPS not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!data)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!last_valid_data.valid)
    {
        return ESP_ERR_NOT_FOUND;
    }

    *data = last_valid_data;
    return ESP_OK;
}

esp_err_t gps_neo6m_deinit(void)
{
    if (!initialized)
    {
        return ESP_OK;
    }

    esp_err_t err = uart_driver_delete(gps_uart_num);
    if (err == ESP_OK)
    {
        initialized = false;
        gps_uart_num = -1;
        ESP_LOGI(TAG, "GPS deinitialized");
    }
    return err;
}

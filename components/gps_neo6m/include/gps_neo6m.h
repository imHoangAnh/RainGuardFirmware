/**
 * @file gps_neo6m.h
 * @brief NEO-6M GPS Module Driver (NMEA Parser)
 */

#ifndef GPS_NEO6M_H
#define GPS_NEO6M_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPS data structure
 */
typedef struct {
    bool valid;           // GPS fix valid
    double latitude;      // Latitude in degrees
    double longitude;     // Longitude in degrees
    float altitude;       // Altitude in meters
    float speed;          // Speed in km/h
    float course;         // Course over ground in degrees
    uint8_t satellites;   // Number of satellites
    uint8_t hour;         // UTC time - hours
    uint8_t minute;       // UTC time - minutes
    uint8_t second;       // UTC time - seconds
} gps_data_t;

/**
 * @brief Initialize GPS module
 * @param uart_num UART port number
 * @param tx_pin GPIO pin for TX
 * @param rx_pin GPIO pin for RX
 * @param baud_rate Baud rate (typically 9600 for NEO-6M)
 * @return ESP_OK on success
 */
esp_err_t gps_neo6m_init(int uart_num, int tx_pin, int rx_pin, int baud_rate);

/**
 * @brief Read and parse GPS data
 * @param data Pointer to GPS data structure
 * @param timeout_ms Timeout in milliseconds
 * @return ESP_OK on success
 */
esp_err_t gps_neo6m_read(gps_data_t *data, uint32_t timeout_ms);

/**
 * @brief Get last valid GPS data
 * @param data Pointer to GPS data structure
 * @return ESP_OK if data is valid
 */
esp_err_t gps_neo6m_get_last_data(gps_data_t *data);

/**
 * @brief Deinitialize GPS module
 * @return ESP_OK on success
 */
esp_err_t gps_neo6m_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // GPS_NEO6M_H


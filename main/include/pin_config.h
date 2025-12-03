/**
 * @file pin_config.h
 * @brief Hardware Pin Configuration for ESP32-S3-WROOM N16R8 (Standard DevKit)
 *
 * Layout Strategy:
 * - LEFT SIDE: Dedicated to Camera (DVP Interface)
 * - RIGHT SIDE: Dedicated to Sensors & Actuators (I2C, GPS, Relay)
 * This separates high-speed camera signals from sensitive sensor data.
 */

#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

#ifdef __cplusplus      
extern "C"
#endif

// ============================================================================
// CAMERA PINS (ESP32-S3 Eye / Freenove S3 Compatible)
// ============================================================================
// Occupies most pins on the LEFT header of the board
#define CAM_PIN_PWDN -1  // Power down (not used)
#define CAM_PIN_RESET -1 // Reset (not used)
#define CAM_PIN_XCLK 15  // GPIO 15 (Left)
#define CAM_PIN_SIOD 4   // GPIO 4  (Left)
#define CAM_PIN_SIOC 5   // GPIO 5  (Left)

#define CAM_PIN_D7 16   // GPIO 16 (Left)
#define CAM_PIN_D6 17   // GPIO 17 (Left)
#define CAM_PIN_D5 18   // GPIO 18 (Left)
#define CAM_PIN_D4 12   // GPIO 12 (Left)
#define CAM_PIN_D3 10   // GPIO 10 (Left)
#define CAM_PIN_D2 8    // GPIO 8  (Left)
#define CAM_PIN_D1 9    // GPIO 9  (Left)
#define CAM_PIN_D0 11   // GPIO 11 (Left)
#define CAM_PIN_VSYNC 6 // GPIO 6  (Left)
#define CAM_PIN_HREF 7  // GPIO 7  (Left)
#define CAM_PIN_PCLK 13 // GPIO 13 (Left)

// ============================================================================
// I2C PINS (For BME680 and MPU6050 Sensors)
// ============================================================================
// RIGHT Side, Top
#define I2C_SDA_PIN 1 // GPIO 1
#define I2C_SCL_PIN 2 // GPIO 2

// ============================================================================
// UART PINS (For GPS NEO-6M)
// ============================================================================
// RIGHT Side, Below I2C (Moving away from Camera pins 3/14 for safety)
#define GPS_UART_NUM UART_NUM_1
#define GPS_UART_TX 42 // Connect to GPS RX
#define GPS_UART_RX 41 // Connect to GPS TX
#define GPS_BAUD_RATE 9600

// ============================================================================
// RELAY CONTROL PIN
// ============================================================================
// RIGHT Side, Bottom
#define RELAY_PIN 21 // GPIO 21

    // ============================================================================
    // UNUSED / FREE PINS (RIGHT SIDE)
    // ============================================================================
    // GPIO 40, 39, 38, 37, 36, 35, 0, 45, 48, 47
    // GPIO 14 (Left, Bottom) - Free if not used by Camera PCLK (we use 13)

#ifdef __cplusplus
}
#endif

#endif // PIN_CONFIG_H

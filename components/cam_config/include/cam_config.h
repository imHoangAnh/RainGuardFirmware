/**
 * @file cam_config.h
 * @brief ESP32 Camera Configuration and Initialization
 */

#ifndef CAM_CONFIG_H
#define CAM_CONFIG_H

#include "esp_camera.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize camera with default configuration
 * @return ESP_OK on success
 */
esp_err_t cam_config_init(void);

/**
 * @brief Deinitialize camera
 * @return ESP_OK on success
 */
esp_err_t cam_config_deinit(void);

/**
 * @brief Capture a single frame
 * @return Pointer to camera frame buffer, NULL on error
 */
camera_fb_t* cam_config_capture(void);

/**
 * @brief Return frame buffer to driver
 * @param fb Pointer to frame buffer
 */
void cam_config_return_fb(camera_fb_t *fb);

/**
 * @brief Get camera sensor pointer for advanced configuration
 * @return Pointer to sensor structure
 */
sensor_t* cam_config_get_sensor(void);

#ifdef __cplusplus
}
#endif

#endif // CAM_CONFIG_H


/**
 * @file cam_config.c
 * @brief ESP32 Camera Configuration Implementation
 */

#include "cam_config.h"
#include "esp_log.h"

static const char *TAG = "CAM_CONFIG";

// Pin definitions for ESP32-S3 Eye / Freenove S3 Compatible boards
#define CAM_PIN_PWDN    -1     // Power down (not used)
#define CAM_PIN_RESET   -1     // Reset (not used)
#define CAM_PIN_XCLK    15     // Master clock
#define CAM_PIN_SIOD    4      // I2C SDA (camera config)
#define CAM_PIN_SIOC    5      // I2C SCL (camera config)

#define CAM_PIN_D7      16     // Data bit 7
#define CAM_PIN_D6      17     // Data bit 6
#define CAM_PIN_D5      18     // Data bit 5
#define CAM_PIN_D4      12     // Data bit 4
#define CAM_PIN_D3      10     // Data bit 3
#define CAM_PIN_D2      8      // Data bit 2
#define CAM_PIN_D1      9      // Data bit 1
#define CAM_PIN_D0      11     // Data bit 0
#define CAM_PIN_VSYNC   6      // Vertical sync
#define CAM_PIN_HREF    7      // Horizontal reference
#define CAM_PIN_PCLK    13     // Pixel clock

esp_err_t cam_config_init(void)
{
    ESP_LOGI(TAG, "Initializing camera...");

    camera_config_t config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,

        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        // Clock configuration
        .xclk_freq_hz = 20000000,        // 20MHz
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        // Image format - JPEG for HTTP upload efficiency
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_SVGA,    // 800x600 (good balance for upload)
        .jpeg_quality = 12,              // 0-63, lower = higher quality
        .fb_count = 2,                   // Double buffering for smooth capture
        .fb_location = CAMERA_FB_IN_PSRAM,  // Use PSRAM for frame buffers
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    // Initialize camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x (%s)", err, esp_err_to_name(err));
        return err;
    }

    // Get sensor for additional configuration
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        ESP_LOGE(TAG, "Failed to get camera sensor");
        return ESP_FAIL;
    }

    // Configure sensor settings for optimal outdoor image quality
    s->set_brightness(s, 0);     // -2 to 2 (0 = default)
    s->set_contrast(s, 0);       // -2 to 2 (0 = default)
    s->set_saturation(s, 0);     // -2 to 2 (0 = default)
    s->set_whitebal(s, 1);       // 0 = disable, 1 = enable (auto white balance)
    s->set_awb_gain(s, 1);       // 0 = disable, 1 = enable (auto white balance gain)
    s->set_wb_mode(s, 0);        // 0 to 4 - white balance mode
    s->set_exposure_ctrl(s, 1);  // 0 = disable, 1 = enable (auto exposure)
    s->set_aec2(s, 0);           // 0 = disable, 1 = enable (auto exposure control 2)
    s->set_ae_level(s, 0);       // -2 to 2 (auto exposure level)
    s->set_aec_value(s, 300);    // 0 to 1200 (manual exposure value)
    s->set_gain_ctrl(s, 1);      // 0 = disable, 1 = enable (auto gain)
    s->set_agc_gain(s, 0);       // 0 to 30 (manual gain)
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6 (gain ceiling)
    s->set_bpc(s, 0);            // 0 = disable, 1 = enable (black pixel correction)
    s->set_wpc(s, 1);            // 0 = disable, 1 = enable (white pixel correction)
    s->set_raw_gma(s, 1);        // 0 = disable, 1 = enable (gamma correction)
    s->set_lenc(s, 1);           // 0 = disable, 1 = enable (lens correction)
    s->set_hmirror(s, 0);        // 0 = disable, 1 = enable (horizontal mirror)
    s->set_vflip(s, 0);          // 0 = disable, 1 = enable (vertical flip)
    s->set_dcw(s, 1);            // 0 = disable, 1 = enable (downsize enable)
    s->set_colorbar(s, 0);       // 0 = disable, 1 = enable (test pattern)

    ESP_LOGI(TAG, "Camera initialized successfully (SVGA, JPEG, PSRAM)");
    ESP_LOGI(TAG, "Sensor PID: 0x%02X", s->id.PID);
    
    return ESP_OK;
}

esp_err_t cam_config_deinit(void)
{
    esp_err_t err = esp_camera_deinit();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Camera deinitialized");
    }
    return err;
}

camera_fb_t* cam_config_capture(void)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return NULL;
    }
    ESP_LOGI(TAG, "Image captured: %zu bytes", fb->len);
    return fb;
}

void cam_config_return_fb(camera_fb_t *fb)
{
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

sensor_t* cam_config_get_sensor(void)
{
    return esp_camera_sensor_get();
}


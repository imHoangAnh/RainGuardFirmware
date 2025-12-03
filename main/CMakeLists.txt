idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES 
        nvs_flash
        cam_config
        app_network
        system_i2c
        sensor_bme680
        sensor_mpu6050
        gps_neo6m
        esp_psram
)

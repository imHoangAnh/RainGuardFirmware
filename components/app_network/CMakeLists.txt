idf_component_register(
    SRCS "app_network.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_http_client nvs_flash esp_netif mqtt
)


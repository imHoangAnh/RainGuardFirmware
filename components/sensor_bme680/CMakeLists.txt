idf_component_register(
    SRCS "sensor_bme680.c"
    INCLUDE_DIRS "include"
    REQUIRES driver system_i2c
)


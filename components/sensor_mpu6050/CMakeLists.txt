idf_component_register(
    SRCS "sensor_mpu6050.c"
    INCLUDE_DIRS "include"
    REQUIRES driver system_i2c
)


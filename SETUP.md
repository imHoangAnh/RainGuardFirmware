# Hướng Dẫn Cài Đặt & Kết Nối Phần Cứng (RainGuard Ver2)

Tài liệu này hướng dẫn cách kết nối dây cho **ESP32-S3-WROOM N16R8 DevKit** (Header 2 hàng chân chuẩn).

---

## 1. Chiến Lược Đi Dây (Layout Strategy)

Để tránh nhiễu và đi dây gọn gàng:
*   **Hàng Bên Trái (Left Header)**: Dành riêng cho **CAMERA**.
*   **Hàng Bên Phải (Right Header)**: Dành riêng cho **CẢM BIẾN & RELAY**.

## 2. Sơ Đồ Chi Tiết (Wiring Table)

### A. Camera (OV2640)
Cắm các chân Camera vào **Hàng Bên Trái** của ESP32-S3 theo thứ tự pin trong `pin_config.h` (nếu dùng module camera rời từng chân).
*Nếu dùng mạch có socket camera tích hợp thì bỏ qua bước này.*

### B. Bus I2C (BME680 & MPU6050) - Hàng Phải, Trên

| Chân ESP32-S3 | Vị trí trên Board | Module BME680/MPU6050 |
| :--- | :--- | :--- |
| **GPIO 1** | Hàng Phải (Trên) | **SDA** |
| **GPIO 2** | Hàng Phải (Dưới chân 1) | **SCL** |
| **3V3** | Hàng Trái (Trên cùng) | **VCC / VIN** |
| **GND** | Hàng Phải (Dưới cùng) | **GND** |

### C. GPS NEO-6M (UART) - Hàng Phải, Giữa
*Lưu ý: Đấu chéo TX-RX*

| Chân ESP32-S3 | Vị trí trên Board | Module GPS |
| :--- | :--- | :--- |
| **GPIO 42** | Hàng Phải (Dưới chân 2) | **RX** (Nhận của GPS) |
| **GPIO 41** | Hàng Phải (Dưới chân 42) | **TX** (Gửi của GPS) |
| **5V** | Hàng Trái (Dưới cùng) | **VCC** |
| **GND** | Hàng Phải (Dưới cùng) | **GND** |

### D. Relay (Điều khiển) - Hàng Phải, Dưới

| Chân ESP32-S3 | Vị trí trên Board | Module Relay |
| :--- | :--- | :--- |
| **GPIO 21** | Hàng Phải (Gần dưới cùng) | **IN / SIG** |
| **5V** | Hàng Trái (Dưới cùng) | **VCC / DC+** |
| **GND** | Hàng Phải (Dưới cùng) | **GND / DC-** |

---

## 3. Hình Minh Họa Vị Trí Chân

```text
       [USB PORT - BOTTOM]
LEFT SIDE (Camera)        RIGHT SIDE (Sensors)
   5V  [ ]                   [ ] GND
  G14  [ ]                   [x] G21  -> RELAY
  G13  [ ]                   [ ] G47
  G12  [ ]                   [ ] G48
  G11  [ ]                   [ ] G45
  G10  [ ]                   [ ] G0
   G9  [ ]                   [ ] G35
   G3  [ ]                   [ ] G36
   G8  [ ]                   [ ] G37
  G18  [ ]                   [ ] G38
  G17  [ ]                   [ ] G39
  G16  [ ]                   [ ] G40
  G15  [ ]                   [x] G41  -> GPS TX
   G7  [ ]                   [x] G42  -> GPS RX
   G6  [ ]                   [x] G2   -> I2C SCL
   G5  [ ]                   [x] G1   -> I2C SDA
   G4  [ ]                   [ ] G44
  RST  [ ]                   [ ] G43
  3V3  [ ]                   [ ] TX
       [ANTENNA - TOP]       [ ] RX
```

## 4. Cấu Hình Code (`main/include/pin_config.h`)

Đã được cập nhật tự động trong code:

```c
#define I2C_SDA_PIN     1
#define I2C_SCL_PIN     2
#define GPS_UART_TX     42
#define GPS_UART_RX     41
#define RELAY_PIN       21
```

## 5. Build & Upload

1.  Kết nối cáp USB.
2.  Chạy lệnh:
    ```bash
    idf.py build
    idf.py -p COMx flash monitor
    ```


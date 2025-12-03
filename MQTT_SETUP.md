# MQTT Sensor Data Test Setup Guide

This guide explains how to set up and test the full sensor data flow via MQTT.

---

## 1. Prerequisites

### Hardware Connected:
- BME680 (Temperature, Humidity, Pressure, Gas) → I2C (GPIO 1, 2)
- MPU6050 (Accelerometer, Gyroscope) → I2C (GPIO 1, 2)
- GPS NEO-6M → UART (GPIO 42, 41)

### Software Required:
- **Mosquitto MQTT Broker** installed on your PC
- ESP32-S3 connected to the same WiFi network as your PC

---

## 2. Install Mosquitto Broker (On Your PC)

### Windows:
1. Download from: https://mosquitto.org/download/
2. Install and start the service:
   ```cmd
   net start mosquitto
   ```

### Linux/Mac:
```bash
sudo apt install mosquitto mosquitto-clients  # Ubuntu/Debian
brew install mosquitto                        # Mac

# Start broker
mosquitto -v
```

---

## 3. Configure WiFi and MQTT in Code

### Edit `components/app_network/app_network.c`:

Find these lines (around line 20-22):

```c
#define WIFI_SSID      "YOUR_WIFI_SSID"
#define WIFI_PASS      "YOUR_WIFI_PASS"
```

**Replace with your actual WiFi credentials.**

### Edit `main/main.c`:

Find this line (around line 28):

```c
#define MQTT_BROKER_URI     "mqtt://192.168.1.100:1883"
```

**Replace `192.168.1.100` with your PC's actual IP address.**

#### How to find your PC's IP:

**Windows:**
```cmd
ipconfig
```
Look for `IPv4 Address` under your WiFi adapter.

**Linux/Mac:**
```bash
ifconfig  # or ip addr
```
Look for `inet` address (e.g., 192.168.1.xxx).

---

## 4. Build and Flash

```bash
idf.py build
idf.py -p COMx flash monitor
```

---

## 5. Monitor MQTT Messages

Open a new terminal on your PC and subscribe to the topic:

```bash
mosquitto_sub -h localhost -t "train/data/ESP32_Train_01" -v
```

You should see JSON messages every 5 seconds:

```json
{
  "deviceId": "ESP32_Train_01",
  "temp": 25.34,
  "hum": 52.10,
  "pressure": 1013.25,
  "gas": 50000,
  "lat": 21.028511,
  "lng": 105.804817,
  "speed": 0.00,
  "vibration": 0.05,
  "accel_x": 0.02,
  "accel_y": -0.01,
  "accel_z": 1.00
}
```

---

## 6. Expected Serial Monitor Output

```text
I (123) MAIN: WiFi Connected, IP: 192.168.1.150
I (456) MAIN: MQTT Connected to broker
I (789) MAIN: BME680 initialized
I (790) MAIN: MPU6050 initialized
I (791) MAIN: GPS initialized
I (5000) MAIN: 📊 Sensor Data: {"deviceId":"ESP32_Train_01",...}
I (5001) MAIN: ✓ Published to MQTT topic: train/data/ESP32_Train_01
```

---

## 7. Troubleshooting

### WiFi not connecting:
- Double-check SSID and password in `app_network.c`
- Check if ESP32 is within WiFi range
- Try `idf.py monitor` to see WiFi error logs

### MQTT not connecting:
- Verify your PC's IP address is correct in `main.c`
- Check if Mosquitto is running: `netstat -an | grep 1883` (should show LISTENING)
- Try disabling PC firewall temporarily
- Test broker: `mosquitto_sub -h YOUR_PC_IP -t test`

### No sensor data:
- Check wiring according to `SETUP.md`
- If sensor init fails, code will use **placeholder data** (still publishes to MQTT)
- Check I2C connections for BME680/MPU6050
- Check UART connections for GPS (TX/RX might be swapped)

### GPS shows placeholder coordinates:
- GPS needs clear sky view and ~30 seconds for first fix
- Placeholder coordinates are Hanoi, Vietnam (21.028511, 105.804817)
- Check if GPS module LED is blinking (indicates searching for satellites)

---

## 8. Data Fields Explained

| Field | Source | Description |
| :--- | :--- | :--- |
| `deviceId` | Hardcoded | ESP32_Train_01 |
| `temp` | BME680 | Temperature (°C) |
| `hum` | BME680 | Humidity (%) |
| `pressure` | BME680 | Atmospheric pressure (hPa) |
| `gas` | BME680 | Gas resistance (Ohms) |
| `lat` | GPS | Latitude (decimal degrees) |
| `lng` | GPS | Longitude (decimal degrees) |
| `speed` | GPS | Speed (km/h) |
| `vibration` | MPU6050 | Calculated from accelerometer magnitude |
| `accel_x/y/z` | MPU6050 | Acceleration (g) |

---

## 9. Next Steps

Once MQTT test is working:
1. ✅ Verify all sensors report real data (not placeholders)
2. Add camera image capture
3. Implement HTTP upload for images
4. Add relay control logic
5. Implement data logging to SPIFFS

---

## 10. MQTT Topic Structure

Current:
```
train/data/ESP32_Train_01
```

Future expansion:
```
train/data/ESP32_Train_01         → Sensor telemetry
train/image/ESP32_Train_01        → Camera images (Base64)
train/status/ESP32_Train_01       → Device status/heartbeat
train/command/ESP32_Train_01      → Remote commands (relay control)
```


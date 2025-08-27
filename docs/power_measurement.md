# Measuring ESP32-CAM Power Consumption with a Multimeter

## Basic Setup for Current Measurement

1. **Connect Multimeter in Series**:
   ```
   Power Source → Multimeter (set to mA) → ESP32-CAM
   ```

2. **Setup Steps**:
   - Set multimeter to DC current measurement mode (200mA or higher range)
   - Disconnect ESP32-CAM power supply
   - Connect multimeter's COM port to power supply ground
   - Connect multimeter's mA port to ESP32-CAM VIN
   - Connect power supply positive to multimeter's input



## Advanced Measurement Options

For more precise measurements:

1. **INA219 Current Sensor**:
   - Connect between power source and ESP32-CAM
   - Add I²C connection to another microcontroller for logging
   - Offers much higher sampling rate than manual readings

2. **USB Power Meter**:
   - If using USB power, insert a USB power meter
   - Can provide continuous voltage/current readings

These methods will give you the actual power consumption, which will be more accurate than software estimates from `software_power_monitor.h`.


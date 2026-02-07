# I2C Slave Test Device for RP2040

## Project Purpose
I2C slave device to receive and display FFT data packets from the audio beamforming master device on a 3.5" TFT display.

## Hardware Configuration

### I2C Communication
- **I2C Port**: I2C1 (moved from I2C0)
- **SDA**: GPIO 26
- **SCL**: GPIO 27
- **Base Address**: 0x60
- **Address Range**: 0x60-0x67 (selectable via buttons)
- **Packet Format**: 41 bytes (0xAA header + 20 frequency bins × 2 bytes)

### TFT Display (ST7796SU1 Controller)
- **Resolution**: 320x480 pixels (used in landscape: 480x320)
- **Interface**: SPI
- **GPIO Pins**:
  - GPIO 2 = SCLK
  - GPIO 3 = MOSI
  - GPIO 4 = MISO
  - GPIO 5 = CS (Chip Select)
  - GPIO 6 = DC (Data/Command)
  - GPIO 7 = RST (Reset)

### Touch Screen (Reserved for Future Use)
- **Interface**: I2C0
- **SDA**: GPIO 8
- **SCL**: GPIO 9
- **TPRST**: GPIO 10
- **TPINT**: GPIO 11

### Button Controls
- **GPIO 14**: Address Up (increment I2C address)
- **GPIO 15**: Address Down (decrement I2C address)
- Pull-up resistors enabled
- Debounce: 200ms

## Features

### Display Visualization
1. **Current I2C Address**: Displayed at top-left in yellow (0x60-0x67)
2. **Frequency Spectrum**: 20 vertical bar graphs showing FFT bin values
   - Bars arranged in 2 rows of 10
   - Color coding: Blue (low freq), Green (mid freq), Red (high freq)
   - Range: 500-5000 Hz
   - Max value: 4095 (12-bit ADC)
3. **Legend**: Shows frequency range information

### Dynamic Address Selection
- Use buttons on GPIO 14/15 to cycle through I2C addresses (0x60-0x67)
- Address change updates I2C hardware configuration and display in real-time
- Allows testing different audio beam directions without reprogramming

## Development Status
Fully implemented with ST7796 display driver and visualization.

## How it works (summary)

- I2C1 slave (GPIO 26/27) receives a 41-byte packet: header 0xAA + 20 bins (16-bit big-endian).
- Buttons on GPIO 14/15 change the active I2C address (0x60–0x67).
- The TFT renders 20 bars in two rows with color bands (low=blue, mid=green, high=red).
- Serial output prints the received bin values for validation.

## Frequency bin centers (Hz)

Derived from $f_s=16000$, $N=256$, start bin 8 (500 Hz), with 20 output bins linearly interpolated across the 500–5000 Hz range.

| Bin | Center (Hz) |
| --- | ----------: |
| 0 | 500 |
| 1 | 725 |
| 2 | 950 |
| 3 | 1175 |
| 4 | 1400 |
| 5 | 1625 |
| 6 | 1850 |
| 7 | 2075 |
| 8 | 2300 |
| 9 | 2525 |
| 10 | 2750 |
| 11 | 2975 |
| 12 | 3200 |
| 13 | 3425 |
| 14 | 3650 |
| 15 | 3875 |
| 16 | 4100 |
| 17 | 4325 |
| 18 | 4550 |
| 19 | 4775 |

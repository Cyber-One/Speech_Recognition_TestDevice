# I2C Slave Test Device for RP2040

## Project Purpose
I2C slave device to receive and display FFT data packets from the audio beamforming master device on a 3.5" TFT display.

## Hardware Configuration

### I2C Communication
- **I2C Port**: I2C0
- **SDA**: GPIO 20
- **SCL**: GPIO 21
- **Base Address**: 0x60
- **Address Range**: 0x60-0x67 (selectable via buttons)
- **Packet Format**: 41 bytes (0xAA header + 40 frequency bins × 1 byte)

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
2. **Frequency Spectrum**: 40-column spectrogram waterfall showing FFT bin values
  - Columns fill the full 480px width (12px per bin)
   - Color coding: Blue (low freq), Green (mid freq), Red (high freq)
  - Range: 500-5500 Hz
  - 8-bit magnitudes (0-255) after right shift
3. **Legend**: Shows frequency range information

### Dynamic Address Selection
- Use buttons on GPIO 14/15 to cycle through I2C addresses (0x60-0x67)
- Address change updates I2C hardware configuration and display in real-time
- Allows testing different audio beam directions without reprogramming

## Development Status
Fully implemented with ST7796 display driver and visualization.

## How it works (summary)

- I2C0 slave (GPIO 20/21) receives a 41-byte packet: header 0xAA + 40 bins (8-bit).
- Buttons on GPIO 14/15 change the active I2C address (0x60–0x67).
- The TFT renders a 40-bin spectrogram with column dividers for clarity.
- Serial output prints the received bin values for validation.

## Frequency bin centers (Hz)

Derived from $f_s=16000$, $N=256$, start bin 8 (500 Hz), with 40 output bins formed by summing pairs of FFT bins across the 500–5500 Hz range. Approximate bin centers are:

$$f_{bin}(n) = 500 + 125n\;\text{Hz},\quad n=0\ldots39$$

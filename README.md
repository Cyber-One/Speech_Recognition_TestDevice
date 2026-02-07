# I2C Test Device with TFT Display

## Overview
This device receives FFT frequency analysis data via I2C from the Speech Recognition AudioCapture master device and displays it as a real-time frequency spectrum on a 3.5" TFT display.

## Hardware Requirements
- Raspberry Pi Pico (RP2040)
- 3.5" TFT Display with ST7796SU1 controller (320x480 resolution)
- 2 push buttons for address selection
- Connections as specified in the pin configuration below

## Pin Configuration

### TFT Display (SPI)
| GPIO | Function | Description |
|------|----------|-------------|
| 2    | SCLK     | SPI Clock |
| 3    | MOSI     | Master Out Slave In |
| 4    | MISO     | Master In Slave Out |
| 5    | CS       | Chip Select |
| 6    | DC       | Data/Command select |
| 7    | RST      | Reset |

### Touch Screen (Reserved)
| GPIO | Function | Description |
|------|----------|-------------|
| 8    | SDA      | Touch I2C Data |
| 9    | SCL      | Touch I2C Clock |
| 10   | TPRST    | Touch Reset |
| 11   | TPINT    | Touch Interrupt |

### Button Controls
| GPIO | Function | Description |
|------|----------|-------------|
| 14   | BTN_UP   | Increment I2C address (pull-up) |
| 15   | BTN_DOWN | Decrement I2C address (pull-up) |

### I2C Audio Data (Slave)
| GPIO | Function | Description |
|------|----------|-------------|
| 26   | SDA      | I2C1 Data (from audio master) |
| 27   | SCL      | I2C1 Clock (from audio master) |

## Features

### Real-Time Frequency Display
- 20 vertical bar graphs representing frequency bins (500-5000 Hz)
- Color-coded by frequency range:
  - **Blue**: Low frequencies (bins 0-6)
  - **Green**: Mid frequencies (bins 7-13)
  - **Red**: High frequencies (bins 14-19)
- Bars update in real-time as audio data is received

### Dynamic Address Selection
- I2C address range: 0x60 - 0x67 (8 addresses for 8 beam angles × 5 from master + 3 extra)
- Press **GPIO 14 button** to increment address
- Press **GPIO 15 button** to decrement address
- Current address displayed at top-left of screen in yellow
- 200ms debounce prevents accidental multiple presses

### Data Reception
- Receives 41-byte packets from audio beamforming master:
  - 1 byte header (0xAA)
  - 20 frequency bins × 2 bytes each (16-bit values)
- I2C slave mode on I2C1 @ 400 kHz
- Interrupt-driven reception for reliable data capture

## Building the Project

### Prerequisites
- Raspberry Pi Pico SDK installed
- CMake and build tools configured
- VS Code with Raspberry Pi Pico extension (recommended)

### Compile
```bash
cd I2C_TestDevice
mkdir -p build
cd build
cmake ..
ninja
```

Or use VS Code task: **Ctrl+Shift+B** → "Compile Project"

### Flash to Pico
1. Hold BOOTSEL button on Pico
2. Connect USB cable
3. Copy `I2C_TestDevice.uf2` from build folder to RPI-RP2 drive

Or use picotool:
```bash
picotool load build/I2C_TestDevice.uf2 -fx
```

## Usage

1. **Power on** the device
2. Display shows "I2C FFT Display" during initialization
3. **Default address** is 0x60 (beam -60° from master)
4. **Press buttons** to change address and select different beam angles:
   - 0x60 = -60° beam
   - 0x61 = -30° beam
   - 0x62 = 0° beam (center)
   - 0x63 = +30° beam
   - 0x64 = +60° beam
5. **Connect I2C** from audio capture master (SDA=GPIO26, SCL=GPIO27)
6. **Observe frequency bars** update in real-time

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

## Display Layout

```
┌─────────────────────────────────────────────────┐
│ I2C: 0x60                    (yellow, size 2)  │
│                                                  │
│ Frequency Spectrum            (white, size 2)   │
│                                                  │
│ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐       │
│ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │  Row 1│
│ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │       │
│ │█│ │█│ │█│ │█│ │█│ │█│ │█│ │█│ │█│ │█│       │
│ │█│ │█│ │█│ │█│ │█│ │█│ │█│ │█│ │█│ │█│       │
│ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘       │
│                                                  │
│ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐       │
│ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │  Row 2│
│ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │ │       │
│ │█│ │█│ │█│ │█│ │█│ │█│ │█│ │█│ │█│ │█│       │
│ │█│ │█│ │█│ │█│ │█│ │█│ │█│ │█│ │█│ │█│       │
│ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘       │
│                                                  │
│ 500-5000 Hz (20 bins)         (gray, size 1)   │
└─────────────────────────────────────────────────┘
```

## Troubleshooting

### Display not working
- Check SPI connections (GPIO 2-7)
- Verify power supply is adequate (TFT can draw significant current)
- Check CS, DC, and RST pins are correctly connected

### No I2C data received
- Verify I2C connections on GPIO 26 (SDA) and 27 (SCL)
- Ensure pull-up resistors on I2C bus (usually 4.7kΩ)
- Check master device is transmitting to correct address
- Use USB serial monitor to see debug output (115200 baud)

### Buttons not responding
- Check buttons are connected between GPIO 14/15 and GND
- Verify pull-ups are enabled (done in software)
- Wait 200ms between button presses (debounce delay)

### Address won't change
- Ensure buttons are properly debounced
- Check serial output for address change confirmation
- Verify address is within range 0x60-0x67

## Serial Debug Output
Connect USB cable and open serial monitor at 115200 baud to see:
- Startup messages
- Current I2C address
- Received packet information
- Frequency bin values
- Address change confirmations

## Technical Details

### ST7796 Display Driver
- Custom driver implementation in `st7796_driver.c`
- SPI communication @ 32 MHz
- 16-bit color (RGB565)
- Hardware-accelerated rectangle fills
- Simple 5x7 bitmap font for text

### Performance
- Display update on packet reception (triggered by interrupt)
- Non-blocking visualization updates
- Smooth bar graph animations
- Typical update rate: matches master transmission rate (~200 Hz)

## Compatible With
- Pico Breadboard Kit Plus Version (as referenced in design)
- Any RP2040 board with sufficient GPIO pins
- Standard 3.5" ST7796-based TFT displays

## License
See LICENSE file in parent directory.

## References
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [ST7796 Datasheet](https://www.displayfuture.com/Display/datasheet/controller/ST7796s.pdf)
- Main Audio Capture Project: `../Speech_Recognition_AudioCapture/`

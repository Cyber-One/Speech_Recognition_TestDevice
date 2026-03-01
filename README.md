# I2C Test Device with TFT Display

## Overview

This device receives FFT frequency analysis data via I2C from the Speech Recognition AudioCapture master device and displays it as a real-time frequency spectrum on a 3.5" TFT display.

## Hardware Requirements

- Raspberry Pi Pico 2 (RP2350)
- 3.5" TFT Display with ST7796SU1 controller (320x480 resolution)
- 2 push buttons for address selection
- Connections as specified in the pin configuration below

## Build Target

- Firmware target board: `PICO_BOARD=pico2`
- MCU platform: `RP2350` (`rp2350-arm-s`)
- Generated output files in `build/` (including `.uf2`, `.elf`, `.bin`, `.hex`) are built for Pico 2 / RP2350.

## Pin Configuration

### TFT Display (SPI)

| GPIO | Function | Description         |
|------|----------|---------------------|
| 2    | SCLK     | SPI Clock           |
| 3    | MOSI     | Master Out Slave In |
| 4    | MISO     | Master In Slave Out |
| 5    | CS       | Chip Select         |
| 6    | DC       | Data/Command select |
| 7    | RST      | Reset               |

### Touch Screen (Reserved)

| GPIO | Function | Description     |
|------|----------|-----------------|
| 8    | SDA      | Touch I2C Data  |
| 9    | SCL      | Touch I2C Clock |
| 10   | TPRST    | Touch Reset     |
| 11   | TPINT    | Touch Interrupt |

### Button Controls

| GPIO | Function | Description                     |
|------|----------|---------------------------------|
| 14   | BTN_UP   | Increment I2C address (pull-up) |
| 15   | BTN_DOWN | Decrement I2C address (pull-up) |

### I2C Link to Speech_Recognition_AudioCapture (Master)

Use these direct wire connections between the two Pico boards:

| Speech_Recognition_AudioCapture (Master) | I2C_TestDevice (Slave) | Purpose |
|------------------------------------------|--------------------------|---------|
| GPIO 8 (I2C SDA)                         | GPIO 20 (I2C SDA)        | I2C data |
| GPIO 9 (I2C SCL)                         | GPIO 21 (I2C SCL)        | I2C clock |
| GND                                      | GND                      | Common reference (required) |

Optional/Recommended:
- Add 4.7kΩ pull-up resistors from SDA to 3.3V and SCL to 3.3V if your bus does not already have pull-ups.
- Keep wiring short (preferably <30 cm) for stable 400 kHz operation.
- Use 3.3V logic only (RP2350 and RP2040 are 3.3V compatible on I/O).

### Dynamic Address Selection

- I2C address range: 0x60 - 0x67
- Press **GPIO 14 button** to increment address
- Receives 41-byte packets from audio beamforming master:
  - 1 byte header (0xAA)
  - 40 frequency bins × 1 byte each (8-bit values)
- I2C slave mode on I2C0 @ 400 kHz
- Raspberry Pi Pico SDK installed
- CMake and build tools configured

```bash
cd I2C_TestDevice
mkdir -p build
cd build
cmake ..
ninja
```

Or use VS Code task: **Ctrl+Shift+B** → "Compile I2C_TestDevice"

### Flash to Pico

1. Hold BOOTSEL button on Pico
2. Connect USB cable
3. Copy `I2C_TestDevice.uf2` from build folder to RPI-RP2 drive

Or use picotool:

```bash
picotool load build/I2C_TestDevice.uf2 -fx
```

## Usage

### Quick Start (Beginner)

1. Build and flash `I2C_TestDevice` firmware to the slave Pico.
2. Build and flash `Speech_Recognition_AudioCapture` firmware to the master Pico.
3. Wire I2C exactly as shown in **I2C Link to Speech_Recognition_AudioCapture (Master)**.
4. Confirm both boards share **GND**.
5. Power both boards.
6. Set test device address with buttons:
  - Start with `0x60` to view the `-60°` beam.
7. Verify display updates when master is running and transmitting.

### Normal Operation

1. **Power on** the device
2. Display shows "I2C FFT Display" during initialization
3. **Default address** is 0x60 (beam -60° from master)
4. **Press buttons** to change address and select different beam angles:
   - 0x60 = -60° beam
   - 0x61 = -30° beam
   - 0x62 = 0° beam (center)
   - 0x63 = +30° beam
   - 0x64 = +60° beam
5. **Connect I2C** from audio capture master (Master GPIO8→Slave GPIO20, Master GPIO9→Slave GPIO21, and GND↔GND)
6. **Observe frequency bars** update in real-time

## How it works (summary)

- I2C0 slave (GPIO 20/21) receives a 41-byte packet: header 0xAA + 40 bins (8-bit).
- Buttons on GPIO 14/15 change the active I2C address (0x60–0x67).
- The TFT renders a 40-bin spectrogram with column dividers.
- Serial output prints the received bin values for validation.

## Frequency bin centers (Hz)

Derived from $f_s=16000$, $N=256$, start bin 8 (500 Hz), with 40 output bins formed by summing pairs of FFT bins across the 500–5500 Hz range. Approximate bin centers are:

$$f_{bin}(n) = 500 + 125n\;\text{Hz},\quad n=0\ldots39$$

### 40 Bin Center Frequencies (Hz)

| Bin | Center (Hz) | Bin | Center (Hz) | Bin | Center (Hz) | Bin | Center (Hz) |
| --- | ----------: | --- | ----------: | --- | ----------: | --- | ----------: |
| 0 | 500 | 10 | 1750 | 20 | 3000 | 30 | 4250 |
| 1 | 625 | 11 | 1875 | 21 | 3125 | 31 | 4375 |
| 2 | 750 | 12 | 2000 | 22 | 3250 | 32 | 4500 |
| 3 | 875 | 13 | 2125 | 23 | 3375 | 33 | 4625 |
| 4 | 1000 | 14 | 2250 | 24 | 3500 | 34 | 4750 |
| 5 | 1125 | 15 | 2375 | 25 | 3625 | 35 | 4875 |
| 6 | 1250 | 16 | 2500 | 26 | 3750 | 36 | 5000 |
| 7 | 1375 | 17 | 2625 | 27 | 3875 | 37 | 5125 |
| 8 | 1500 | 18 | 2750 | 28 | 4000 | 38 | 5250 |
| 9 | 1625 | 19 | 2875 | 29 | 4125 | 39 | 5375 |

## Display Layout

┌─────────────────────────────────────────────────┐
│ I2C: 0x60                    (yellow, size 2)   │
│                                                 │
│ Frequency Spectrum            (white, size 2)   │
│                                                 │
│ │││││││││││││││││││││││││││││││││││││││││       │
│ │││││││││││││││││││││││││││││││││││││││││       │
│ (40-bin spectrogram with column dividers)       │
│                                                 │
│ 500-5500 Hz (40 bins)         (gray, size 1)    │
└─────────────────────────────────────────────────┘

## Troubleshooting

- Verify power supply is adequate (TFT can draw significant current)
- Check CS, DC, and RST pins are correctly connected

### No I2C data received

- Verify cross-device wiring: Master GPIO8→Slave GPIO20 and Master GPIO9→Slave GPIO21
- Verify both devices share a common GND
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

- Display update throttled to once every 2 seconds to reduce SPI load
- Non-blocking visualization updates

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

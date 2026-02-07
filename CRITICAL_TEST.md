# CRITICAL TROUBLESHOOTING - Firmware Not Running

## Problem Identified
Your Pico is not running the firmware at all:
- LED doesn't blink (code isn't executing)
- No serial output (USB not initializing)
- Display backlight is on (power is good)

## Minimal Test Firmware

I've created a super simple test firmware that ONLY does:
1. Blink LED 5 times immediately (before USB)
2. Initialize USB
3. Blink LED 5 more times
4. Print messages
5. Blink continuously every second

**Location**: `build/test_minimal.uf2`

### How to Flash Test Firmware

1. **Hold BOOTSEL button** on Pico
2. **Connect USB cable**
3. **Release BOOTSEL** - Pico appears as drive "RPI-RP2"
4. **Drag `test_minimal.uf2`** from build folder to the drive
5. Pico will reboot automatically

### What Should Happen

**Immediately after flashing:**
- LED should blink 5 times rapidly (1 second total)
- Short pause (3 seconds for USB)
- LED blinks 5 more times
- LED continues blinking every second forever

**Serial Output** (115200 baud on COM6):
```
=== MINIMAL TEST FIRMWARE ===
If you see this, USB serial is working!
LED should be blinking continuously...

LED ON  - Count: 0
LED OFF - Count: 0
LED ON  - Count: 1
LED OFF - Count: 1
...
```

## If LED Still Doesn't Blink

This would indicate:
1. **Pico hardware fault** - very rare but possible
2. **Wrong UF2 file** - make sure you're using the newly built one
3. **Power issue** - try different USB port/cable
4. **Bootloader corrupted** - Pico might need firmware recovery

### Pico Recovery Mode

If normal BOOTSEL doesn't work:
1. **Disconnect USB**
2. **Hold BOOTSEL button**
3. **While holding**, connect USB
4. **Keep holding** for 3 seconds
5. **Release** - should appear as RPI-RP2 drive

## If LED DOES Blink But No Serial

This would indicate:
1. **USB cable is charge-only** (no data lines) - very common!
2. **USB driver issue** - try different computer
3. **Serial port issue** - check Device Manager for COM port

### Check USB Cable
- Try a different USB cable (many are power-only)
- Use a cable you know works for data (like phone sync cable)
- Avoid cheap charging cables

## Next Steps

1. **Flash `test_minimal.uf2`**
2. **Watch the LED** - this is the most important test!
3. **Report results**:
   - Does LED blink immediately after flash?
   - How many blinks do you see?
   - Does it continue blinking?
   - Any serial output?

### Why This Test Matters

The minimal test has NO display code, no I2C code, just LED and serial. If this doesn't work, the problem is:
- Hardware (Pico, USB cable, computer port)
- Build system (UF2 file)
- NOT the display driver

If minimal test WORKS (LED blinks), then we know:
- Pico hardware is good
- Build system is good  
- Problem is in the display initialization code
- We can debug from there

## File Locations

- **Minimal Test**: `C:\Users\rayed\Code\Speech_Capture\Audio_Capture\I2C_TestDevice\build\test_minimal.uf2`
- **File Size**: 67,072 bytes
- **Built**: February 1, 2026

Make sure you're flashing THIS file, not the old I2C_TestDevice.uf2!

---

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

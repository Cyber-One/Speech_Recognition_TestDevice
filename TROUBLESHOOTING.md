# Display Troubleshooting Guide

## Current Issue: Display Not Lighting Up

### New Debug Build

A new version has been compiled with extensive debug output. Flash this to your Pico:
- **File**: `build/I2C_TestDevice.uf2`
- **Build time**: Just rebuilt with debug features

### What to Check via Serial Output

Connect a serial terminal (115200 baud) to see detailed startup information:

1. **LED Blinks**: 3 blinks on startup (shows code is running)
2. **Display Initialization**: Step-by-step SPI and display setup
3. **Test Pattern**: Colored rectangles should appear for 2 seconds
4. **Component Status**: All peripherals report initialization

### Expected Serial Output

```
=== I2C Slave Test Device with TFT Display ===
Firmware starting...
Pico SDK Version: x.x.x
Blinking LED to show startup...

--- Display Initialization ---
Initializing ST7796 display...
SPI Pins: SCLK=2, MOSI=3, MISO=4, CS=5, DC=6, RST=7
ST7796: Initializing SPI...
ST7796: SPI initialized at 32 MHz
ST7796: Initializing control pins (CS=5, DC=6, RST=7)
ST7796: Control pins configured
ST7796: Performing hardware reset...
ST7796: Hardware reset complete
ST7796: Sending software reset command...
ST7796: Waking display from sleep...
ST7796: Setting 16-bit color mode...
ST7796: Configuring memory access...
ST7796: Configuring inversion...
ST7796: Turning display ON...
ST7796: Display initialization complete!
Display initialized
Rotation set to landscape
Screen cleared to black
Drawing startup text...
Clearing screen again...
Display initialization complete!

--- Display Test Pattern ---
Drawing test pattern...
Test pattern complete
Test pattern displayed for 2 seconds...
Test pattern cleared

--- Button Initialization ---
...
```

## Common Display Issues

### 1. Display Stays Black
**Possible causes:**
- Wrong pin connections
- Display needs more power
- Display backlight not enabled
- Wrong display controller (not ST7796)

**Check:**
- Verify all 7 wires (SCLK, MOSI, MISO, CS, DC, RST + power/GND)
- Try powering display from external 5V source (not 3.3V)
- Check if display has separate backlight enable pin

### 2. Display Flickers or Shows Garbage
**Possible causes:**
- SPI clock too fast
- Poor connections/long wires
- Insufficient power

**Try:**
- Reduce SPI speed in `st7796_driver.c` line 172: change `32 * 1000 * 1000` to `16 * 1000 * 1000`
- Use shorter wires (under 10cm)
- Add 100µF capacitor near display power pins

### 3. Test Pattern Shows But Text Doesn't
- Display is working! Text rendering may need adjustment
- Check serial output for drawing commands

### 4. No Serial Output at All
- USB cable might be charge-only (no data)
- Try different USB cable
- Check if LED blinks (code is running even without serial)

## Pin Verification Table

Double-check your wiring:

| Pico GPIO | Display Pin | Function |
|-----------|-------------|----------|
| GPIO 2 | SCK/CLK | SPI Clock |
| GPIO 3 | MOSI/SDA | SPI Data Out |
| GPIO 4 | MISO | SPI Data In (can leave NC if no touch) |
| GPIO 5 | CS | Chip Select |
| GPIO 6 | DC/RS | Data/Command |
| GPIO 7 | RST/RESET | Reset |
| 3.3V | VCC | Power (or 5V if available) |
| GND | GND | Ground |
| GPIO ? | LED/BL | Backlight (if separate pin) |

**Note**: Some displays have the backlight permanently on, others need a separate enable pin or PWM control.

## Testing Steps

### Step 1: Load New Debug Firmware
1. Hold BOOTSEL button
2. Connect USB
3. Copy `I2C_TestDevice.uf2` to RPI-RP2 drive
4. Pico will reboot automatically

### Step 2: Observe Onboard LED
- Should blink 3 times on startup
- If it doesn't blink: firmware didn't load or Pico is faulty

### Step 3: Connect Serial Terminal
Open any serial terminal at 115200 baud:
- Windows: PuTTY, TeraTerm, Arduino Serial Monitor
- Terminal: `screen COM6 115200` (replace COM6 with your port)

### Step 4: Watch for Test Pattern
After initialization messages, display should show:
- Red, Green, Blue squares across top
- Yellow, Cyan, Magenta across middle
- White, Gray, Black across bottom
- Pattern shows for 2 seconds then clears

### Step 5: Report What You See
Note which of these happen:
- [ ] LED blinks 3 times
- [ ] Serial output appears
- [ ] Test pattern appears on display
- [ ] Display backlight turns on
- [ ] Any flickering or partial display

## Alternative: Backlight Issue

Many ST7796 displays have a separate backlight control. If you see the test pattern VERY faintly (need dark room), the backlight might not be enabled.

**Solutions:**
1. **Check for BL/LED pin**: Some displays have separate backlight pin
   - Connect to 3.3V through 100Ω resistor (or direct if marked "BL")
2. **Jumper setting**: Some displays have jumper to enable backlight
3. **PWM control**: Some need PWM signal on backlight pin

## Next Steps

After flashing the new debug firmware:
1. **Open serial terminal** (115200 baud)
2. **Copy all the serial output** and share it
3. **Note what you see** on the display (even if very faint)
4. **Check LED behavior** (blinks 3 times?)
5. **Verify wiring** against the pin table above

The debug output will tell us exactly where the problem is!

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

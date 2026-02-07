# Hardware Connection Diagram

## I2C Test Device - Quick Pin Reference

```
┌─────────────────────────────────────────────────────────────────┐
│                     Raspberry Pi Pico (RP2040)                   │
│                                                                   │
│  TFT Display (SPI)          Touch Screen (Reserved)              │
│  ┌──────────────┐           ┌──────────────┐                    │
│  │ GPIO 2 SCLK  │           │ GPIO 8  SDA  │                    │
│  │ GPIO 3 MOSI  │           │ GPIO 9  SCL  │                    │
│  │ GPIO 4 MISO  │           │ GPIO 10 TPRST│                    │
│  │ GPIO 5 CS    │           │ GPIO 11 TPINT│                    │
│  │ GPIO 6 DC    │           └──────────────┘                    │
│  │ GPIO 7 RST   │                                                │
│  └──────────────┘           I2C Audio (Slave)                    │
│                             ┌──────────────┐                     │
│  Button Controls            │ GPIO 26 SDA  │ ← From Master      │
│  ┌──────────────┐           │ GPIO 27 SCL  │ ← From Master      │
│  │ GPIO 14 UP   │           └──────────────┘                     │
│  │ GPIO 15 DOWN │                                                │
│  └──────────────┘                                                │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

## I2C Address Selection

Press buttons to select which audio beam to monitor:

| Address | Beam Angle | Button Sequence |
|---------|------------|-----------------|
| 0x60    | -60°       | Default (or DOWN from 0x61) |
| 0x61    | -30°       | UP once from 0x60 |
| 0x62    | 0° (center)| UP twice from 0x60 |
| 0x63    | +30°       | UP 3× from 0x60 |
| 0x64    | +60°       | UP 4× from 0x60 |
| 0x65-0x67| Reserved  | UP 5-7× from 0x60 |

## Display Layout (480x320 pixels, Landscape)

```
┌────────────────────────────────────────────────────────────────┐
│ I2C: 0x62                                                      │ ← Current address
│                                                                │
│ Frequency Spectrum                                             │ ← Title
│                                                                │
│ [BAR1][BAR2][BAR3][BAR4][BAR5][BAR6][BAR7][BAR8][BAR9][BAR10]│ ← Row 1 (bins 0-9)
│   █     █     █     █     █     █     █     █     █     █    │
│   █     █     █     █     █     █     █     █     █     █    │
│   █     █     █     █     █     █     █     █     █     █    │
│   █     █     █     █     █     █     █     █     █     █    │
│  Blue  Blue  Blue   ←─── Green ───→  ←────── Red ──────→    │ ← Color coding
│                                                                │
│ [BAR11][BAR12][BAR13][BAR14][BAR15][BAR16][BAR17][BAR18]...  │ ← Row 2 (bins 10-19)
│   █     █     █     █     █     █     █     █     █     █    │
│   █     █     █     █     █     █     █     █     █     █    │
│   █     █     █     █     █     █     █     █     █     █    │
│   █     █     █     █     █     █     █     █     █     █    │
│                                                                │
│ 500-5000 Hz (20 bins)                                         │ ← Legend
└────────────────────────────────────────────────────────────────┘
```

## Wiring Notes

### TFT Display Power
- **VCC**: 3.3V (100-200mA peak)
- **GND**: Ground
- Add 10µF capacitor near display for stability

### I2C Bus
- **Pull-ups**: 4.7kΩ on SDA and SCL (may be built into master)
- **Cable length**: Keep under 30cm for 400kHz operation
- **Termination**: Not required at this speed

### Buttons
- **Configuration**: Active-low with internal pull-ups
- **Wiring**: Button between GPIO and GND
- **Debounce**: 200ms in software (no external caps needed)

## Power Supply Requirements

| Component | Current | Notes |
|-----------|---------|-------|
| RP2040 Core | ~30mA | Typical operation |
| TFT Display | 100-200mA | Peak during full-screen updates |
| Total | ~250mA | Use quality 5V USB power |

**Recommendation**: Use USB power supply rated for at least 500mA.

## Troubleshooting LED Patterns

This device uses the Pico's onboard LED for status:
- **Solid ON**: Initialization complete, waiting for data
- **Flickering**: Receiving I2C packets (normal operation)
- **Off**: Device not powered or failed to initialize

Check USB serial output (115200 baud) for detailed status.

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

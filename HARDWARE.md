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
│  Button Controls            │ GPIO 20 SDA  │ ← From Master      │
│  ┌──────────────┐           │ GPIO 21 SCL  │ ← From Master      │
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
│ ││││││││││││││││││││││││││││││││││││││││││ │
│ │││││││││││││││││││││││││││││││││││││││││ │
│ (40-bin spectrogram with column dividers)                      │
│                                                                │
│ 500-5500 Hz (40 bins)                                         │ ← Legend
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

- I2C0 slave (GPIO 20/21) receives a 41-byte packet: header 0xAA + 40 bins (8-bit).
- Buttons on GPIO 14/15 change the active I2C address (0x60–0x67).
- The TFT renders a 40-bin spectrogram with column dividers.
- Serial output prints the received bin values for validation.

## Frequency bin centers (Hz)

Derived from $f_s=16000$, $N=256$, start bin 8 (500 Hz), with 40 output bins formed by summing pairs of FFT bins across the 500–5500 Hz range. Approximate bin centers are:

$$f_{bin}(n) = 500 + 125n\;\text{Hz},\quad n=0\ldots39$$

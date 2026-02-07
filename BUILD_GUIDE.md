# Building the I2C_TestDevice Project

This document explains how to build the I2C_TestDevice in a multi-folder workspace.

## The Issue

When you have both `Speech_Recognition_AudioCapture` and `I2C_TestDevice` in the same VS Code workspace, pressing **Ctrl+Shift+B** defaults to building the first project's tasks.

## Solutions

### Option 1: Using VS Code Tasks (Easiest)

1. Press **Ctrl+Shift+P** (Command Palette)
2. Type: `Tasks: Run Build Task`
3. Select: **"Compile I2C_TestDevice"**

Or press **Ctrl+Shift+B** and choose from the task list.

### Option 2: Using the PowerShell Build Script (Fastest)

```powershell
# From the I2C_TestDevice folder:
.\build.ps1
```

This script automatically:
- Creates the build directory if needed
- Runs CMake configuration
- Compiles with Ninja
- Shows the output file location

### Option 3: Manual Terminal Build

```powershell
# Navigate to the project folder
cd C:\Users\rayed\Code\Speech_Capture\Audio_Capture\I2C_TestDevice

# Create and navigate to build directory (first time only)
mkdir build
cd build
cmake ..

# Build (every time you make changes)
cmake --build .

# Or use Ninja directly (faster)
~\.pico-sdk\ninja\v1.12.1\ninja.exe -C build
```

### Option 4: CMake Tools Extension

If you have the **CMake Tools** extension installed:

1. Look at the **bottom status bar**
2. Click the **folder/project icon** (shows current project name)
3. Select **I2C_TestDevice**
4. Click the **Build** button in the status bar

## Workspace Configuration

The workspace now has separate `tasks.json` files:

- **Speech_Recognition_AudioCapture/.vscode/tasks.json** - Builds the main audio project
- **I2C_TestDevice/.vscode/tasks.json** - Builds the I2C test device

## Output Files

After building, you'll find:

```
I2C_TestDevice/
├── build/
│   ├── I2C_TestDevice.uf2    ← Flash this to your Pico
│   ├── I2C_TestDevice.elf    ← For debugging
│   └── I2C_TestDevice.dis    ← Disassembly
```

## Flashing to Pico

### Method 1: Drag and Drop
1. Hold **BOOTSEL** button on Pico
2. Connect USB cable
3. Release BOOTSEL (Pico appears as drive)
4. Drag `I2C_TestDevice.uf2` to the drive

### Method 2: Using picotool
```powershell
picotool load build/I2C_TestDevice.uf2 -fx
```

## Quick Reference

| Action | Command |
|--------|---------|
| First build | `cd I2C_TestDevice; mkdir build; cd build; cmake ..` |
| Rebuild | `ninja -C build` from I2C_TestDevice folder |
| Clean build | Delete `build` folder and rebuild |
| Build script | `.\build.ps1` from I2C_TestDevice folder |
| VS Code task | Ctrl+Shift+P → "Tasks: Run Build Task" |

## Troubleshooting

### "Wrong project builds when I press Ctrl+Shift+B"
- Use **Ctrl+Shift+P** → **"Tasks: Run Build Task"** instead
- Select the specific project task you want

### "CMake can't find source files"
- Make sure you're in the correct project folder
- Check that `CMakeLists.txt` exists in the current directory

### "Ninja not found"
- The script uses: `~\.pico-sdk\ninja\v1.12.1\ninja.exe`
- Verify this path exists
- If not, adjust the path in `tasks.json` or use `cmake --build .` instead

### "Build succeeds but wrong .uf2 created"
- Check which folder you're in: `pwd`
- Make sure you're building from `I2C_TestDevice` not `Speech_Recognition_AudioCapture`
- Check the terminal output to see which CMakeLists.txt is being used

## File Locations Summary

```
Audio_Capture/
├── Speech_Recognition_AudioCapture/    ← Main audio capture project
│   ├── build/
│   │   └── Speech_Recognition_AudioCapture.uf2
│   └── .vscode/tasks.json
│
└── I2C_TestDevice/                     ← Display test device project
    ├── build/
    │   └── I2C_TestDevice.uf2         ← You want this one!
    ├── .vscode/tasks.json
    └── build.ps1                       ← Quick build script
```

## Recommended Workflow

For regular development:

1. Open terminal in I2C_TestDevice folder
2. Run `.\build.ps1` after making changes
3. Flash `build\I2C_TestDevice.uf2` to your Pico

This avoids confusion between the two projects!

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

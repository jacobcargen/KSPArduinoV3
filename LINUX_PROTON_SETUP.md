# KSP Simpit Setup for Linux (Proton/Steam)

## Quick Fix for Keyboard Emulation

The keyboard emulation wasn't working because Proton/Wine needs access to your X11 display to send keypresses.

### Solution: Add Launch Options in Steam

1. **Right-click KSP in Steam** → Properties → Launch Options

2. **Add this line (includes X11 access + ultrawide resolution):**
   ```
   DISPLAY=:0 xhost +local: && %command% -screen-width 3840 -screen-height 1080 -screen-fullscreen
   ```

3. **Save and launch KSP**

That's it! The keyboard emulation should now work.

---

## Full Setup Guide

### 1. Serial Port Configuration

Your Arduino is at `/dev/ttyACM0` and is already mapped to **COM33** in Wine.

In your KSP Settings.cfg file:
```
KerbalSimpit
{
    Documentation = https://kerbalsimpitrevamped-arduino.readthedocs.io
    Verbose = True
    RefreshRate = 125
    SerialPort
    {
        PortName = COM33
        BaudRate = 115200
    }
}
```

**Settings.cfg location:**
- Usually: `[KSP Install]/GameData/KerbalSimpit/Settings.cfg`
- Copy from: `GameData/KerbalSimpit/PluginData/Settings.cfg.sample` if needed

### 2. Verify COM Port Mapping

The COM33 → /dev/ttyACM0 mapping should already exist at:
```
~/.local/share/Steam/steamapps/compatdata/220200/pfx/dosdevices/com33
```

If it doesn't exist, run:
```bash
ln -sf /dev/ttyACM0 ~/.local/share/Steam/steamapps/compatdata/220200/pfx/dosdevices/com33
```

### 3. Enable Keyboard Emulation (X11)

**Method 1 - Steam Launch Options (Recommended):**
```
DISPLAY=:0 xhost +local: && %command%
```

**Method 2 - Run before launching KSP:**
```bash
export DISPLAY=:0
xhost +local:
```

### 4. Verify It's Working

After launching KSP, check the log file:
```
~/.local/share/Steam/steamapps/common/Kerbal Space Program/KSP.log
```

Look for these messages:
- `KerbalSimpit: Settings loaded.`
- `KerbalSimpit: Using X11 keyboard simulation (Linux)`
- `KerbalSimpit: Starting poll thread for port COM33`
- Connection status messages

### 5. Arduino Upload

Your Arduino code is already configured for 115200 baud (matches Settings.cfg).

Upload to Arduino Due:
```bash
cd /home/jacob-dev/repos/KSPArduinoV3
./upload_arduino.sh
```

### 6. Test Keyboard Emulation

Once connected in KSP, test these keys from your Arduino controller:
- F2: Toggle UI (should work)
- M: Toggle Map View (should work with X11)
- V: Cycle Camera (should work with X11)
- C: Toggle IVA/External (should work with X11)
- ESC: Pause (should work with X11)
- All numpad keys for warning cancellation

### Troubleshooting

**Problem: "No X11 libraries found"**
- Install: `sudo apt-get install libx11-6 libxtst6`

**Problem: "X11 keycode is 0"**
- Some keys may not have proper mapping
- Check KSP.log for warnings about unmapped keys

**Problem: Serial port not opening**
- Check permissions: `ls -la /dev/ttyACM0`
- Add user to dialout: `sudo usermod -a -G dialout $USER` (then log out/in)

**Problem: Keys still not working**
- Make sure `xhost +local:` was run
- Check if DISPLAY is set: `echo $DISPLAY`
- Try `DISPLAY=:1` if :0 doesn't work

**Problem: Works in native Linux but not Proton**
- This is why you need the DISPLAY variable in launch options
- Proton isolates Wine from X11 by default

### Security Note

`xhost +local:` allows local connections to your X server. This is generally safe but you can disable it after closing KSP with:
```bash
xhost -local:
```

Or use the more secure approach in Steam launch options which runs it only when KSP launches.

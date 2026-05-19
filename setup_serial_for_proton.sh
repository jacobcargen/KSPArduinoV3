#!/bin/bash
# Script to create a Windows-style COM port for KSP running under Proton
# This allows the KerbalSimpit plugin to communicate with the Arduino

# Configuration
LINUX_PORT="/dev/ttyACM0"  # Change this if your Arduino is on a different port
COM_PORT="COM3"            # The Windows COM port name to use

# Find your Steam compatdata directory
# This is usually in ~/.local/share/Steam/steamapps/compatdata/220200/pfx/dosdevices/
STEAM_COMPAT_DIR="$HOME/.local/share/Steam/steamapps/compatdata/220200/pfx/dosdevices"

# Alternative location if using Flatpak Steam
FLATPAK_COMPAT_DIR="$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/compatdata/220200/pfx/dosdevices"

# Detect which directory exists
if [ -d "$STEAM_COMPAT_DIR" ]; then
    DOSDEVICES_DIR="$STEAM_COMPAT_DIR"
    echo "Found Steam compatdata directory: $DOSDEVICES_DIR"
elif [ -d "$FLATPAK_COMPAT_DIR" ]; then
    DOSDEVICES_DIR="$FLATPAK_COMPAT_DIR"
    echo "Found Flatpak Steam compatdata directory: $DOSDEVICES_DIR"
else
    echo "ERROR: Could not find Steam compatdata directory!"
    echo "Please manually set the DOSDEVICES_DIR variable in this script."
    echo "It should point to: [Steam directory]/steamapps/compatdata/220200/pfx/dosdevices/"
    exit 1
fi

# Check if Arduino is connected
if [ ! -e "$LINUX_PORT" ]; then
    echo "ERROR: Arduino not found at $LINUX_PORT"
    echo "Available serial ports:"
    ls -la /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || echo "No serial ports found"
    exit 1
fi

# Check if user has permission to access the serial port
if [ ! -r "$LINUX_PORT" ] || [ ! -w "$LINUX_PORT" ]; then
    echo "ERROR: No permission to access $LINUX_PORT"
    echo "Adding user to dialout group..."
    sudo usermod -a -G dialout $USER
    echo "You must log out and log back in for this to take effect!"
    exit 1
fi

# Create symlink in dosdevices directory
COM_LINK="$DOSDEVICES_DIR/$(echo $COM_PORT | tr '[:upper:]' '[:lower:]')"

# Remove old symlink if it exists
if [ -L "$COM_LINK" ]; then
    echo "Removing old symlink: $COM_LINK"
    rm "$COM_LINK"
fi

# Create new symlink
echo "Creating symlink: $COM_LINK -> $LINUX_PORT"
ln -s "$LINUX_PORT" "$COM_LINK"

# Verify the link was created
if [ -L "$COM_LINK" ]; then
    echo "SUCCESS! Serial port mapped."
    echo ""
    echo "In your KSP Settings.cfg file, use:"
    echo "  PortName = $COM_PORT"
    echo ""
    echo "Location of Settings.cfg:"
    echo "  [KSP Install]/GameData/KerbalSimpit/Settings.cfg"
    echo ""
    echo "Make sure it looks like this:"
    echo "----------------------------------------"
    echo "KerbalSimpit"
    echo "{"
    echo "    Documentation = https://kerbalsimpitrevamped-arduino.readthedocs.io"
    echo "    Verbose = True"
    echo "    RefreshRate = 125"
    echo "    SerialPort"
    echo "    {"
    echo "        PortName = $COM_PORT"
    echo "        BaudRate = 115200"
    echo "    }"
    echo "}"
    echo "----------------------------------------"
else
    echo "ERROR: Failed to create symlink"
    exit 1
fi

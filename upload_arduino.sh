#!/bin/bash
# Compile and Upload Arduino Code to Arduino Due
# This script handles the complete Arduino upload process

set -e  # Exit on error

# Configuration
BOARD_FQBN="arduino:sam:arduino_due_x_dbg"
PORT="/dev/ttyACM0"
SKETCH_DIR="$(pwd)"
SKETCH_FILE="KSPArduinoV3.ino"

echo "========================================"
echo "Arduino Due - Compile and Upload"
echo "========================================"
echo ""

# Check if we're in the right directory
if [ ! -f "$SKETCH_FILE" ]; then
    echo "Error: $SKETCH_FILE not found. Are you in the KSPArduinoV3 directory?"
    exit 1
fi

# Check if arduino-cli is installed
if ! command -v arduino-cli &> /dev/null; then
    echo "arduino-cli not found. Please install it:"
    echo "   curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh"
    exit 1
fi

# Check if Arduino Due port exists
if [ ! -e "$PORT" ]; then
    echo "Warning: Arduino Due not found at $PORT"
    echo "Available ports:"
    ls -l /dev/ttyACM* 2>/dev/null || echo "  No /dev/ttyACM* ports found"
    ls -l /dev/ttyUSB* 2>/dev/null || echo "  No /dev/ttyUSB* ports found"
    echo ""
    read -p "Enter the correct port (or press Enter to use $PORT anyway): " USER_PORT
    if [ ! -z "$USER_PORT" ]; then
        PORT="$USER_PORT"
    fi
fi

echo "Configuration:"
echo "  Board: Arduino Due (Programming Port)"
echo "  FQBN: $BOARD_FQBN"
echo "  Port: $PORT"
echo "  Sketch: $SKETCH_FILE"
echo ""

# Compile
echo "========================================"
echo "Compiling sketch..."
echo "========================================"
arduino-cli compile -b "$BOARD_FQBN" --warnings all

COMPILE_STATUS=$?
if [ $COMPILE_STATUS -ne 0 ]; then
    echo ""
    echo "Compilation failed!"
    exit 1
fi

echo ""
echo "Compilation successful"
echo ""

# Check sketch size
SKETCH_SIZE=$(arduino-cli compile -b "$BOARD_FQBN" --warnings none 2>&1 | grep "Sketch uses" | awk '{print $3, $4, $5, $6, $7, $8, $9}')
if [ ! -z "$SKETCH_SIZE" ]; then
    echo "Sketch size: $SKETCH_SIZE"
    echo ""
fi

# Prompt before upload
read -p "Upload to Arduino Due? (y/n) " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Upload cancelled."
    exit 0
fi

# Upload
echo ""
echo "========================================"
echo "Uploading to Arduino Due..."
echo "========================================"
echo "NOTE: Make sure KSP is NOT connected to the serial port!"
echo ""

arduino-cli upload --fqbn "$BOARD_FQBN" -p "$PORT"

UPLOAD_STATUS=$?
if [ $UPLOAD_STATUS -ne 0 ]; then
    echo ""
    echo "Upload failed!"
    echo ""
    echo "Troubleshooting:"
    echo "  - Make sure KSP serial connection is closed"
    echo "  - Check that Arduino Due is connected to $PORT"
    echo "  - Try pressing the reset button on the Arduino"
    exit 1
fi

echo ""
echo "========================================"
echo "UPLOAD COMPLETE!"
echo "========================================"
echo ""
echo "Arduino Due has been programmed successfully."
echo "You can now:"
echo "  1. Restart KSP (if it's running)"
echo "  2. Connect to serial port in KSP"
echo "  3. Test keyboard emulation functions"
echo ""
echo "Keyboard functions enabled:"
echo "  - F1: Screenshot"
echo "  - F2: UI Toggle"
echo "  - M:  Map View Toggle"
echo "  - C:  Camera View Toggle"
echo ""

#!/bin/bash
# Install X11 libraries for KSP Proton prefix to enable keyboard emulation
# This allows KerbalSimpit to use X11 for sending keypresses

STEAM_PREFIX="$HOME/.local/share/Steam/steamapps/compatdata/220200/pfx"

echo "========================================="
echo "Installing X11 libraries for KSP Proton"
echo "========================================="

# Check if winetricks is installed
if ! command -v winetricks &> /dev/null; then
    echo "Installing winetricks..."
    sudo apt-get update
    sudo apt-get install -y winetricks
fi

# Set the Wine prefix for KSP
export WINEPREFIX="$STEAM_PREFIX"

echo ""
echo "Wine prefix: $WINEPREFIX"
echo ""
echo "Installing X11 libraries (libx11, libxtst)..."
echo "This may take a few minutes..."
echo ""

# Install X11 libraries using winetricks
# Note: We need to use Proton's wine, not system wine
PROTON_PATH="$HOME/.local/share/Steam/steamapps/common/Proton 8.0/proton"

if [ ! -f "$PROTON_PATH" ]; then
    echo "Looking for Proton installation..."
    PROTON_PATH=$(find ~/.local/share/Steam/steamapps/common/ -name "proton" -type f 2>/dev/null | head -1)
    if [ -z "$PROTON_PATH" ]; then
        echo "ERROR: Could not find Proton installation"
        echo "Please manually set PROTON_PATH in this script"
        exit 1
    fi
fi

echo "Using Proton: $PROTON_PATH"

# Try to copy system X11 libraries into the Wine prefix
echo ""
echo "Copying system X11 libraries to Wine prefix..."

# Find system X11 libraries
LIBX11=$(find /usr/lib -name "libX11.so.6" 2>/dev/null | head -1)
LIBXTST=$(find /usr/lib -name "libXtst.so.6" 2>/dev/null | head -1)

if [ -z "$LIBX11" ] || [ -z "$LIBXTST" ]; then
    echo "ERROR: X11 libraries not found on system"
    echo "Installing X11 libraries on host system..."
    sudo apt-get install -y libx11-6 libxtst6
    
    # Search again
    LIBX11=$(find /usr/lib -name "libX11.so.6" 2>/dev/null | head -1)
    LIBXTST=$(find /usr/lib -name "libXtst.so.6" 2>/dev/null | head -1)
fi

echo "Found libX11: $LIBX11"
echo "Found libXtst: $LIBXTST"

# Create lib directory in Wine prefix if it doesn't exist
mkdir -p "$WINEPREFIX/drive_c/windows/system32"

# Copy the libraries
echo "Copying libraries..."
cp "$LIBX11" "$WINEPREFIX/drive_c/windows/system32/" 2>/dev/null
cp "$LIBXTST" "$WINEPREFIX/drive_c/windows/system32/" 2>/dev/null

# Also try lib64 for 64-bit
mkdir -p "$WINEPREFIX/drive_c/windows/syswow64"
cp "$LIBX11" "$WINEPREFIX/drive_c/windows/syswow64/" 2>/dev/null
cp "$LIBXTST" "$WINEPREFIX/drive_c/windows/syswow64/" 2>/dev/null

echo ""
echo "========================================="
echo "Installation complete!"
echo "========================================="
echo ""
echo "Now make sure your Steam launch options are set to:"
echo "DISPLAY=:0 xhost +local: && %command% -screen-width 3840 -screen-height 1080 -screen-fullscreen"
echo ""
echo "Then launch KSP and check the log for:"
echo "'Simpit KeyboardEmulator: Using X11 keyboard simulation (Linux)'"
echo ""
echo "If it still says 'X11 libraries not found', the issue is that"
echo "KSP/Mono can't load native .so files from Wine's system32."
echo ""
echo "Alternative solution: Use native Linux KSP instead of Proton version"
echo "========================================="

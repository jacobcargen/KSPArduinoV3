# KSP Proton - Automatic Key Remapping

Since X11 keyboard emulation doesn't work through Proton/Wine, the KerbalSimpit plugin **automatically remaps** your key presses to function keys (F3-F12) which are safe.

## How It Works

- **Native Linux (with X11)**: Your keys work as normal (M, V, C, etc.)
- **Proton/Wine (no X11)**: Keys are automatically remapped to F3-F12

You don't need to change your Arduino code! The plugin handles the remapping automatically.

## Automatic Key Mapping (Proton Mode)

| Original Key | Auto-Mapped To | KSP Action to Bind | 
|--------------|----------------|-------------------|
| M (0x4D) | **F3** (0x72) | Toggle Map View |
| ESC (0x1B) | **F4** (0x73) | Pause |
| V (0x56) | **F5** (0x74) | Cycle Camera Mode |
| C (0x43) | **F6** (0x75) | Toggle IVA/External |
| ` (0xC0) | **F7** (0x76) | Reset Camera |
| ] (0xDD) | **F8** (0x77) | Focus Next Vessel |
| Delete (0x2E) | **F9** (0x78) | Toggle Docking Mode |
| F2 (0x71) | **F2** (0x71) | Toggle UI (no change) |

## Setting Up KSP Controls

In **Settings → Input → Flight**, rebind these keys:

1. **Toggle Map View**: Set to `F3`
2. **Pause**: Set to `F4`
3. **Cycle Camera Mode**: Set to `F5`
4. **Toggle IVA/External**: Set to `F6`
5. **Reset Camera**: Set to `F7`
6. **Focus Next Vessel**: Set to `F8`
7. **Toggle Docking Mode**: Set to `F9`

## Arduino Code

**No changes needed!** Keep using the original key codes:
- 0x4D for Map
- 0x1B for Pause
- 0x56 for Camera Mode
- etc.

The plugin will automatically remap them when running in Proton.

## Notes

- Numpad keys (0x60-0x69) are not mapped in Proton mode
- F1 and F2 already work natively
- On-screen messages show which F-key was triggered
- Check KSP.log to see the remapping in action

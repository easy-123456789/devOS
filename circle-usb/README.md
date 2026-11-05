# USB keyboard support (Raspberry Pi 3B+)

The original freestanding C kernel does not contain a USB host stack. The `circle-usb/` application is an alternate Circle-based firmware build that adds one: Circle supports the Pi 3B+'s DWC2 host controller, its built-in USB hub, and standard HID keyboards. When it detects a keyboard, keypresses are echoed to the HDMI screen. UART logging remains enabled as a diagnostic fallback. This USB keyboard demo replaces the current console firmware when you install its `kernel8.img`; it is not linked into the original C kernel.

## Build

1. Install the AArch64 bare-metal toolchain used by the project (`aarch64-none-elf-gcc` and matching binutils), GNU make, and Git.
2. From the repository root, clone Circle and build its libraries:

   ```sh
   git clone https://github.com/rsta2/circle.git vendor/circle
   cd vendor/circle
   printf 'AARCH = 64\nRASPPI = 3\nPREFIX64 = aarch64-none-elf-\n' > Config.mk
   ./makeall
   cd ../..
   ```

3. Build the USB keyboard firmware:

   ```sh
   make -C circle-usb
   ```

   Circle's make rules produce `circle-usb/kernel8.img`.
4. Copy that image to the Pi boot partition as `kernel8.img` (back up the existing image first) along with the required Raspberry Pi boot firmware files and `config.txt`. Boot with a USB keyboard plugged into one of the Pi's USB-A ports.

The screen should show a ready message and echo typed keys. Use a standard USB HID keyboard; exotic vendor-specific keyboards may not be supported. This firmware is a keyboard-input demonstration, not the existing UART command shell. It replaces that firmware image, so the former shell and its SD `load` command are unavailable while this build is installed. Restore the original `kernel8.img` to return to the C console.

Circle is GPL-3.0; see `vendor/circle/LICENSE`. Circle source is fetched separately rather than copied into this repository.

USB initialization and actual keyboard operation require a Raspberry Pi 3B+ and cannot be verified in this source-only environment. The firmware should be tested on hardware before relying on it.

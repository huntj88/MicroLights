#!/usr/bin/env bash
# Cross-platform DFU flashing using dfu-util (Linux / macOS / Windows via Git Bash or MSYS2).
#
# Requirements:
#   - dfu-util on PATH
#       Linux:   sudo apt install dfu-util   (or your distro's package manager)
#       macOS:   brew install dfu-util
#       Windows: install dfu-util and use the WinUSB/libusb driver (e.g. via Zadig)
#                for the "STM32 BOOTLOADER" device. Run from Git Bash or MSYS2.
#   - Linux only: a udev rule for the bootloader avoids needing sudo. Add it with:
#       echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="df11", MODE="0666"' \
#         | sudo tee -a /etc/udev/rules.d/99-microlight.rules
#       sudo udevadm control --reload-rules && sudo udevadm trigger
#     Then unplug and replug the device.
#
#     Note: this project already relies on a companion rule for the running app
#     device (the TinyUSB firmware, USB VID 0xcafe) so that `pnpm dfu` can talk to
#     it without sudo. That rule is currently only installed on the dev machine
#     (not tracked in the repo); document it here for reproducibility:
#       echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="cafe", MODE="0666"' \
#         | sudo tee -a /etc/udev/rules.d/99-microlight.rules
#     For a clean setup, install both rules at once:
#       printf '%s\n' \
#         'SUBSYSTEM=="usb", ATTR{idVendor}=="cafe", MODE="0666"' \
#         'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="df11", MODE="0666"' \
#         | sudo tee /etc/udev/rules.d/99-microlight.rules
#       sudo udevadm control --reload-rules && sudo udevadm trigger
#
# The STM32 system DFU bootloader enumerates as USB VID:PID 0483:df11,
# alt setting 0 = "Internal Flash". Flash base for the STM32C071 is 0x08000000.

set -euo pipefail

# --- Resolve paths relative to this script ----------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

DFU_VID_PID="0483:df11"
FLASH_ADDR="0x08000000"
FIRMWARE="build/Debug/BulbChipSTM32C071FBPx.bin"

# --- Detect platform --------------------------------------------------------
case "$(uname -s)" in
  Linux*)  PLATFORM="linux" ;;
  Darwin*) PLATFORM="mac" ;;
  MINGW*|MSYS*|CYGWIN*) PLATFORM="windows" ;;
  *)       PLATFORM="unknown" ;;
esac

# --- Locate dfu-util --------------------------------------------------------
if ! command -v dfu-util >/dev/null 2>&1; then
  echo "Error: dfu-util not found on PATH." >&2
  case "$PLATFORM" in
    linux)   echo "Install it with: sudo apt install dfu-util" >&2 ;;
    mac)     echo "Install it with: brew install dfu-util" >&2 ;;
    windows) echo "Install dfu-util and ensure its folder is on PATH (run from Git Bash/MSYS2)." >&2 ;;
  esac
  exit 1
fi

# --- Check firmware exists --------------------------------------------------
if [ ! -f "$FIRMWARE" ]; then
  echo "Error: firmware not found at $FIRMWARE" >&2
  echo "Build it first with: make" >&2
  exit 1
fi

# --- Put the device into DFU mode (best effort) -----------------------------
if (cd ../App/configure-app-v3 && pnpm dfu); then
  sleep 0.5
fi

# --- On Linux, re-exec as root if the bootloader isn't accessible -----------
# (macOS and Windows do not use sudo for USB access.)
if [ "$PLATFORM" = "linux" ] && [ "${EUID:-$(id -u)}" -ne 0 ]; then
  if ! dfu-util -l 2>/dev/null | grep -qi "$DFU_VID_PID"; then
    echo "Bootloader not accessible without elevated permissions; re-running with sudo..."
    echo "(Add a udev rule to avoid this — see the install notes at the top of this script.)"
    exec sudo env PATH="$PATH" bash "$0" "$@"
  fi
fi

# --- List connected DFU devices ---------------------------------------------
echo "Connected DFU devices:"
dfu-util -l || true

# --- Flash and launch the application ---------------------------------------
# -a 0            : alt setting 0 (Internal Flash)
# -s ADDR:leave   : download to ADDR, then exit DFU and run the app
# -D FILE         : download (flash) FILE
#
# Note: with ":leave", the STM32 resets to run the new app before dfu-util can
# read the final status, so it reports "Error during download get_status" and a
# non-zero exit even though the flash completed. Detect that case and treat a
# successful "File downloaded successfully" as success.
set +e
flash_output="$(dfu-util -d "$DFU_VID_PID" -a 0 -s "${FLASH_ADDR}:leave" -D "$FIRMWARE" 2>&1)"
flash_status=$?
set -e
echo "$flash_output"

if [ "$flash_status" -ne 0 ]; then
  if echo "$flash_output" | grep -q "File downloaded successfully"; then
    echo
    echo "Flash complete; device reset to run the application."
    echo "(Ignored harmless 'get_status' error from the DFU leave/reset step.)"
    exit 0
  fi
  echo
  echo "Error: flashing failed." >&2
  exit "$flash_status"
fi

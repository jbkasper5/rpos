#!/bin/bash

# === CONFIG ===
CURR_DIR=$(pwd)
VOLUME_NAME="RPOS"  # Change this to match the name of your SD card
MOUNT_POINT="$CURR_DIR/sdcard"  # Change to your target directory

# === FIND DISK IDENTIFIER ===
DISK_ID=$(diskutil info "$VOLUME_NAME" | grep "Device Node" | awk '{print $3}')

# === ERROR HANDLING ===
if [ -z "$DISK_ID" ]; then
  echo "❌ Could not find disk with volume name '$VOLUME_NAME'."
  exit 1
fi

echo "🔍 Found device: $DISK_ID"

# === UNMOUNT DEFAULT MOUNT POINT ===
echo "🔄 Ejecting disk $DISK_ID..."
diskutil eject $DISK_ID
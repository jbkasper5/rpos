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
echo "🛑 Unmounting from /Volumes/$VOLUME_NAME..."
diskutil unmount "/Volumes/$VOLUME_NAME"

# === CREATE CUSTOM MOUNT DIRECTORY IF NEEDED ===
mkdir -p "$MOUNT_POINT"

# === MOUNT TO CUSTOM LOCATION ===
echo "🔄 Mounting $DISK_ID to $MOUNT_POINT..."
sudo mount -t msdos "$DISK_ID" "$MOUNT_POINT"

echo "✅ Mounted successfully at $MOUNT_POINT"
#!/bin/bash

# === CONFIG ===
CURR_DIR=$(pwd)
VOLUME_NAME="RPOS"          # Change this to match the label of your SD card
MOUNT_POINT="$CURR_DIR/sdcard"  # Target mount directory

# === FIND DISK DEVICE ===
# Find device with matching volume label
DISK_ID=$(lsblk -o NAME,LABEL -P | grep "LABEL=\"$VOLUME_NAME\"" | awk -F'"' '{print "/dev/" $2}')

# === ERROR HANDLING ===
if [ -z "$DISK_ID" ]; then
  echo "❌ Could not find disk with volume name '$VOLUME_NAME'."
  exit 1
fi

echo "🔍 Found device: $DISK_ID"

# === UNMOUNT IF MOUNTED ===
if mount | grep -q "$DISK_ID"; then
  echo "🛑 Unmounting $DISK_ID..."
  sudo umount "$DISK_ID"
fi

# === CREATE CUSTOM MOUNT DIRECTORY IF NEEDED ===
mkdir -p "$MOUNT_POINT"

# === MOUNT TO CUSTOM LOCATION ===
echo "🔄 Mounting $DISK_ID to $MOUNT_POINT..."
sudo mount -o uid=$(id -u),gid=$(id -g) -t vfat "$DISK_ID" "$MOUNT_POINT"


echo "✅ Mounted successfully at $MOUNT_POINT"

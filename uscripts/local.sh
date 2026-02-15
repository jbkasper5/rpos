#!/bin/bash

# Define variables
SD_CARD_DEVICE="/dev/sdX"  # Replace 'sdX' with the actual SD card device
MOUNT_POINT="/mnt/rootfs"
SRC_BINARIES="src/user/bin"
DEST_BINARIES="$MOUNT_POINT/bin"

# Mount the root filesystem
sudo mkdir -p $MOUNT_POINT
sudo mount $SD_CARD_DEVICE $MOUNT_POINT

# Check if the mount was successful
if [ $? -ne 0 ]; then
    echo "Failed to mount the SD card. Please check the device path."
    exit 1
fi

# Copy only files (exclude directories)
find $SRC_BINARIES -maxdepth 1 -type f -exec sudo cp {} $DEST_BINARIES/ \;

# Unmount the SD card
sudo umount $MOUNT_POINT

echo "Binaries copied successfully to the root filesystem."

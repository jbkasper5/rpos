#!/bin/bash

VOLUME_NAME="RPOS"  # SD card label

# Find the parent disk of the partition with this label
DISK_PART=$(lsblk -o NAME,LABEL -P | grep "LABEL=\"$VOLUME_NAME\"" | awk -F'"' '{print "/dev/" $2}')
if [ -z "$DISK_PART" ]; then
    echo "❌ Could not find disk with label '$VOLUME_NAME'."
    exit 1
fi

DISK_NAME=$(lsblk -no PKNAME $DISK_PART)

# Unmount all partitions on the disk
PARTITIONS=$(lsblk -ln -o NAME /dev/$DISK_NAME)
for part in $PARTITIONS; do
    if mount | grep -q "/dev/$part"; then
        echo "🛑 Unmounting /dev/$part..."
        sudo umount "/dev/$part"
    fi
done

# Power off the disk
echo "🔄 Ejecting /dev/$DISK_NAME..."
sudo udisksctl power-off -b "/dev/$DISK_NAME"

echo "✅ Disk /dev/$DISK_NAME ejected successfully."

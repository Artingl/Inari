#!/bin/bash
set -e

# Configuration
IMAGE_NAME="build/boot.img"
IMAGE_SIZE="40M"
MOUNT_DIR="./fs"
KERNEL_BIN="build/kernel.elf" 
GRUB_CFG="grub.cfg"

cleanup() {
    echo ">>> Cleaning up..."
    
    # 1. Unmount if mounted
    # We redirect stderr (2>/dev/null) so it doesn't complain if not mounted
    if mountpoint -q $MOUNT_DIR; then
        umount $MOUNT_DIR
    fi
    
    # 2. Detach loop device ONLY if it still exists
    if [ ! -z "$LOOP_DEV" ]; then
        # Check if the device file actually exists and is a block device
        if [ -b "$LOOP_DEV" ]; then
            losetup -d $LOOP_DEV 2>/dev/null || true
        fi
    fi
    
    # 3. Remove directory
    rm -rf $MOUNT_DIR
    echo ">>> Cleanup Finished."
}

trap cleanup EXIT

# Check root
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (sudo)"
  exit
fi

# 1. Create empty file
echo ">>> Creating disk image..."
rm -f $IMAGE_NAME
truncate -s $IMAGE_SIZE $IMAGE_NAME

# 2. Partitioning
# We need TWO partitions:
#   1. A tiny 1MB partition for GRUB core (No Filesystem, Flag: bios_grub)
#   2. The actual data partition (FAT32)
echo ">>> Partitioning..."

# Create GPT Label
parted -s $IMAGE_NAME mklabel gpt

# Create BIOS Boot Partition (Start 1MB, End 2MB)
parted -s $IMAGE_NAME mkpart primary 1MiB 2MiB
parted -s $IMAGE_NAME set 1 bios_grub on

# Create Data Partition (Start 2MB, End 100%)
parted -s $IMAGE_NAME mkpart primary fat32 2MiB 100%

# 3. Setup Loopback
echo ">>> Setting up loop device..."
LOOP_DEV=$(losetup -P --show -f $IMAGE_NAME)
echo "    Attached to $LOOP_DEV"

# 4. Format Partition 2 (Partition 1 is raw for GRUB)
echo ">>> Formatting FAT32..."
mkfs.fat -F 32 -n "Inari" "${LOOP_DEV}p2"

# 5. Mount Partition 2
echo ">>> Mounting..."
mkdir -p $MOUNT_DIR
mount "${LOOP_DEV}p2" $MOUNT_DIR

# 6. Copy Files
echo ">>> Copying kernel and config..."
mkdir -p $MOUNT_DIR/boot/grub
cp $KERNEL_BIN $MOUNT_DIR/boot/kernel.bin
cp $GRUB_CFG $MOUNT_DIR/boot/grub/grub.cfg
cp test/main.exe $MOUNT_DIR/init.exe

# 7. Install GRUB
# GRUB will automatically find the 'bios_grub' partition (p1) 
# and write its core image there.
echo ">>> Installing GRUB..."
grub-install \
    --target=i386-pc \
    --boot-directory=$MOUNT_DIR/boot \
    --no-floppy \
    --modules="normal part_gpt fat multiboot" \
    $LOOP_DEV

# 8. Cleanup
echo ">>> Cleaning up..."
umount $MOUNT_DIR
losetup -d $LOOP_DEV
rm -rf $MOUNT_DIR

chown -R artingl:artingl $IMAGE_NAME
chown -R artingl:artingl build

echo ">>> Done! $IMAGE_NAME is ready."
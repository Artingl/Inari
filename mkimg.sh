#!/bin/bash
set -e

# Configuration
BOOTABLE_IMAGE_NAME="build/boot.iso"
GRUB_DIR="./grub"
IMAGE_NAME="build/rootfs.img"
IMAGE_SIZE="10M"
MOUNT_DIR="./fs"
KERNEL_BIN="build/kernel.elf"
GRUB_CFG="grub.cfg"

cleanup() {
    if mountpoint -q $MOUNT_DIR; then
        echo ">>> Unmounting..."
        umount $MOUNT_DIR
    fi

    if [ ! -z "$LOOP_DEV" ] && [ -b "$LOOP_DEV" ]; then
        echo ">>> Detaching loop device..."
        losetup -d $LOOP_DEV 2>/dev/null || true
    fi

    rm -rf $MOUNT_DIR



    # Install GRUB
    echo ">>> Installing GRUB..."
    mkdir -p $GRUB_DIR/boot/grub
    cp $KERNEL_BIN $GRUB_DIR/kernel.bin
    cp $GRUB_CFG $GRUB_DIR/boot/grub/grub.cfg
    cp $IMAGE_NAME $GRUB_DIR
    grub-mkrescue -d /usr/lib/grub/i386-pc -o $BOOTABLE_IMAGE_NAME $GRUB_DIR
    chown $SUDO_USER:$SUDO_USER $BOOTABLE_IMAGE_NAME

    rm -rf $GRUB_DIR
}
trap cleanup EXIT

if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (sudo)"
  exit 1
fi

if [ -d "./fs" ]; then
    echo "Unmounting old fs"
    umount -q $(pwd)/fs
    rm -rf $(pwd)/fs
fi

# Create empty file
echo ">>> Creating disk image..."
rm -f $IMAGE_NAME
truncate -s $IMAGE_SIZE $IMAGE_NAME

# Partitioning (MBR)
echo ">>> Partitioning..."

# Create MBR Label
parted -s $IMAGE_NAME mklabel msdos

# Create Primary Partition (FAT32)
parted -s $IMAGE_NAME mkpart primary fat32 1MiB 100%

# Set the 'Boot' flag
parted -s $IMAGE_NAME set 1 boot on

# Setup Loopback
echo ">>> Setting up loop device..."
LOOP_DEV=$(losetup -P --show -f $IMAGE_NAME)
echo "    Attached to $LOOP_DEV"

# Format Partition 1
echo ">>> Formatting FAT32..."
mkfs.fat -F 16 -n "Inari" "${LOOP_DEV}p1"

# Mount Partition 1
echo ">>> Mounting..."
mkdir -p $MOUNT_DIR
mount "${LOOP_DEV}p1" $MOUNT_DIR

# Copy Files
echo ">>> Copying kernel and config..."
mkdir -p $MOUNT_DIR/programs
mkdir -p $MOUNT_DIR/shell
mkdir -p $MOUNT_DIR/system
mkdir -p $MOUNT_DIR/devices
cp -r userspace/fs/* $MOUNT_DIR/
cp userspace/apps/*.exe $MOUNT_DIR/programs 2>/dev/null || :
cp userspace/*.exe $MOUNT_DIR/programs 2>/dev/null || :
cp userspace/shell/*.exe $MOUNT_DIR/shell 2>/dev/null || :
cp motd.txt $MOUNT_DIR/system

# grub-install \
#     --target=i386-pc \
#     --boot-directory=$MOUNT_DIR/boot \
#     --no-floppy \
#     --modules="normal part_msdos fat multiboot" \
#     $LOOP_DEV

# Permissions Fix
if [ ! -z "$SUDO_USER" ]; then
    echo ">>> Fixing permissions for $SUDO_USER..."
    chown $SUDO_USER:$SUDO_USER $IMAGE_NAME
fi

echo ">>> Done! $IMAGE_NAME is ready."

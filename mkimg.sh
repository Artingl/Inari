#!/bin/bash
set -e

# Configuration
IMAGE_NAME="build/boot.img"
IMAGE_SIZE="40M"
MOUNT_DIR="./fs"
KERNEL_BIN="build/kernel.elf" 
GRUB_CFG="grub.cfg"

# --- CLEANUP TRAP ---
cleanup() {
    # Silence cleanup noise unless there is an error
    if mountpoint -q $MOUNT_DIR; then
        echo ">>> Unmounting..."
        umount $MOUNT_DIR
    fi
    
    if [ ! -z "$LOOP_DEV" ] && [ -b "$LOOP_DEV" ]; then
        echo ">>> Detaching loop device..."
        losetup -d $LOOP_DEV 2>/dev/null || true
    fi
    
    rm -rf $MOUNT_DIR
}
trap cleanup EXIT

# --- CHECKS ---
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (sudo)"
  exit 1
fi

# 1. Create empty file
echo ">>> Creating disk image..."
rm -f $IMAGE_NAME
truncate -s $IMAGE_SIZE $IMAGE_NAME

# 2. Partitioning (MBR / MSDOS Standard)
echo ">>> Partitioning (MBR Mode)..."

# Create MBR Label
parted -s $IMAGE_NAME mklabel msdos

# Create Primary Partition (FAT32)
parted -s $IMAGE_NAME mkpart primary fat32 1MiB 100%

# Set the 'Boot' flag
parted -s $IMAGE_NAME set 1 boot on

# 3. Setup Loopback
echo ">>> Setting up loop device..."
LOOP_DEV=$(losetup -P --show -f $IMAGE_NAME)
echo "    Attached to $LOOP_DEV"

# 4. Format Partition 1
echo ">>> Formatting FAT32..."
mkfs.fat -F 32 -n "Inari" "${LOOP_DEV}p1"

# 5. Mount Partition 1
echo ">>> Mounting..."
mkdir -p $MOUNT_DIR
mount "${LOOP_DEV}p1" $MOUNT_DIR

# 6. Copy Files
echo ">>> Copying kernel and config..."
mkdir -p $MOUNT_DIR/boot/grub
cp $KERNEL_BIN $MOUNT_DIR/boot/kernel.bin
cp $GRUB_CFG $MOUNT_DIR/boot/grub/grub.cfg

# Copy your init/test executable if it exists
if [ -f "test/main.exe" ]; then
    cp test/main.exe $MOUNT_DIR/init.exe
fi

mkdir -p $MOUNT_DIR/dev

# 7. Install GRUB
echo ">>> Installing GRUB..."
grub-install \
    --target=i386-pc \
    --boot-directory=$MOUNT_DIR/boot \
    --no-floppy \
    --modules="normal part_msdos fat multiboot" \
    $LOOP_DEV

# 8. Permissions Fix
if [ ! -z "$SUDO_USER" ]; then
    echo ">>> Fixing permissions for $SUDO_USER..."
    chown $SUDO_USER:$SUDO_USER $IMAGE_NAME
fi

echo ">>> Done! $IMAGE_NAME is ready."

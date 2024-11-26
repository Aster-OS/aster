#! /bin/sh

make image-name
IMAGE_NAME=$(cd image-name && echo *)
KERNEL="kernel/bin/kernel"

GDB_PATH="x86_64-elf-gdb"

QEMU_PATH="qemu-system-x86_64"
QEMU_FLAGS="-M q35"
QEMU_USER_FLAGS="-m 2G"

BOOT="bios"
IMAGE_TYPE="iso"

for arg in "$@"; do
    case $arg in
        uefi)
            BOOT="uefi" ;;
        hdd)
            IMAGE_TYPE="hdd" ;;
        kvm)
            KVM=true ;;
        log)
            LOG=true ;;
        serial)
            SERIAL=true ;;
        gdb)
            GDB=true ;;
        *)
            echo "Unknown option: $arg" >&2
            exit 1 ;;
    esac
done

if [ $BOOT = "uefi" ]; then
    QEMU_FLAGS="$QEMU_FLAGS -drive if=pflash,unit=0,format=raw,file=ovmf/ovmf-code-x86_64.fd,readonly=on"
    make ovmf
fi

if [ $IMAGE_TYPE = "iso" ]; then
    QEMU_FLAGS="$QEMU_FLAGS -cdrom $IMAGE_NAME.iso -boot d"
    make all
fi

if [ $IMAGE_TYPE = "hdd" ]; then
    QEMU_FLAGS="$QEMU_FLAGS -hda $IMAGE_NAME.hdd"
    make all-hdd
fi

if [ "$KVM" = true ]; then
    QEMU_FLAGS="$QEMU_FLAGS -enable-kvm"
fi

if [ "$LOG" = true ]; then
    QEMU_FLAGS="$QEMU_FLAGS -D ./qemu.log -d int -M smm=off -no-reboot -no-shutdown"
fi

if [ "$SERIAL" = true ]; then
    QEMU_FLAGS="$QEMU_FLAGS -serial file:qemu-serial.log"
fi

if [ "$GDB" = true ]; then
    QEMU_FLAGS="$QEMU_FLAGS -s -S"
    gnome-terminal --title="GDB (Aster)" -- $GDB_PATH $KERNEL --init-command=gdb_init_cmd.txt
fi

$QEMU_PATH $QEMU_FLAGS $QEMU_USER_FLAGS

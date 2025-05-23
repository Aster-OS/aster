#! /bin/sh

make image-name || exit 1;
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
        debugcon)
            DEBUGCON=true ;;
        serial)
            SERIAL=true ;;
        mp)
            MP=true ;;
        monitor)
            MONITOR=true ;;
        gdb)
            GDB=true ;;
        *)
            echo "Unknown option: $arg" >&2
            exit 1 ;;
    esac
done

if [ $BOOT = "uefi" ]; then
    QEMU_FLAGS="$QEMU_FLAGS -drive if=pflash,unit=0,format=raw,file=ovmf/ovmf-code-x86_64.fd,readonly=on"
    make ovmf || exit 1;
fi

if [ $IMAGE_TYPE = "iso" ]; then
    QEMU_FLAGS="$QEMU_FLAGS -cdrom $IMAGE_NAME.iso -boot d"
    make all || exit 1;
fi

if [ $IMAGE_TYPE = "hdd" ]; then
    QEMU_FLAGS="$QEMU_FLAGS -hda $IMAGE_NAME.hdd"
    make all-hdd || exit 1;
fi

if [ "$KVM" = true ]; then
    QEMU_FLAGS="$QEMU_FLAGS -enable-kvm"
fi

if [ "$LOG" = true ]; then
    QEMU_FLAGS="$QEMU_FLAGS -D ./qemu.log -d int -M smm=off -no-reboot -no-shutdown"
fi

if [ "$DEBUGCON" = true ]; then
    QEMU_FLAGS="$QEMU_FLAGS -debugcon stdio"
fi

if [ "$SERIAL" = true ]; then
    QEMU_FLAGS="$QEMU_FLAGS -serial file:qemu-serial.log"
fi

if [ "$MP" = true ]; then
    QEMU_FLAGS="$QEMU_FLAGS -smp 4"
fi

if [ "$MONITOR" = true ]; then
    QEMU_FLAGS="$QEMU_FLAGS -monitor stdio"
fi

if [ "$GDB" = true ]; then
    QEMU_FLAGS="$QEMU_FLAGS -s -S"
    gnome-terminal --title="GDB (Aster)" -- $GDB_PATH $KERNEL --init-command=gdb_init_cmd.txt
fi

$QEMU_PATH $QEMU_FLAGS $QEMU_USER_FLAGS

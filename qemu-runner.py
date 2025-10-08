#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
from datetime import datetime
from pathlib import Path

def get_log_file_path(subdirectory, filename):
    path = Path("qemu-runner") / subdirectory
    path.mkdir(exist_ok=True, parents=True)
    return path / filename

def main():
    parser = argparse.ArgumentParser(
        description="Run QEMU with various configuration options",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--arch", action="store", choices=["x86_64"], default="x86_64", help="CPU architecture")
    parser.add_argument("--bios", action="store_true", help="boot with legacy BIOS")
    parser.add_argument("--debug", action="store_true", help="run QEMU for debugging with GDB")
    parser.add_argument("--debugcon", action="store", choices=["file", "stdio"], help="enable QEMU debugcon")
    parser.add_argument("--image", action="store", help="image to boot", metavar="FILE")
    parser.add_argument("--kvm", action="store_true", help="enable KVM")
    parser.add_argument("--log", action="store", choices=["file", "stderr"], help="enable QEMU logs")
    parser.add_argument("--monitor", action="store_true", help="enable QEMU monitor (stdio only)")
    parser.add_argument("--serial", action="store", choices=["file", "stdio"], help="enable QEMU serial logging")
    parser.add_argument("--smp", action="store", default="1", help="number of CPUs", metavar="COUNT")
    parser.add_argument("--verbose", action="store_true", help="print the QEMU command line")

    args = parser.parse_args()

    qemu = f"qemu-system-{args.arch}"
    qemu_flags = "-m 2G -vga std".split()

    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

    if args.bios and args.arch != "x86_64":
        print("Error: BIOS boot is only supported for x86_64", file=sys.stderr)
        sys.exit(1)

    # --arch
    if args.arch == "x86_64":
        qemu_flags += "-M q35".split()

    # --bios
    if args.bios:
        qemu_flags += "-boot d".split()
    else:
        qemu_flags += f"-drive if=pflash,unit=0,format=raw,file=ovmf/ovmf-code-{args.arch}.fd,readonly=on".split()

    # --debug
    if args.debug:
        qemu_flags += "-s -S".split()

    # --debugcon
    if args.debugcon == "file":
        log_file = get_log_file_path("debugcon", f"{timestamp}.log")
        qemu_flags += f"-debugcon file:{log_file}".split()
    elif args.debugcon == "stdio":
        qemu_flags += "-debugcon stdio".split()

    # --image
    image = args.image or f"aster-{args.arch}.iso"

    # --image-type
    image_type = Path(image).suffix
    if image_type == ".hdd":
        qemu_flags += f"-hda {image}".split()
    elif image_type == ".iso":
        qemu_flags += f"-cdrom {image}".split()
    else:
        print(f"Error: Invalid image type '{image_type}'", file=sys.stderr)
        print("Error: Supported types are .hdd, .iso", file=sys.stderr)
        sys.exit(1)

    # --kvm
    if args.kvm:
        qemu_flags += "-cpu host -enable-kvm".split()

    # --log
    if args.log == "file":
        log_file = get_log_file_path("log", f"{timestamp}.log")
        qemu_flags += f"-D {log_file}".split()

    if args.log:
        qemu_flags += "-d int -M smm=off -no-reboot -no-shutdown".split()

    # --monitor
    if args.monitor:
        qemu_flags += "-monitor stdio".split()

    # --serial
    if args.serial == "file":
        log_file = get_log_file_path("serial", f"{timestamp}.log")
        qemu_flags += f"-serial file:{log_file}".split()
    elif args.serial == "stdio":
        qemu_flags += "-serial mon:stdio".split()

    # --smp
    if args.smp != "1":
        qemu_flags += f"-smp {args.smp}".split()

    qemu_cmd = [qemu] + qemu_flags

    if args.verbose:
        print(qemu_cmd)

    try:
        subprocess.run(qemu_cmd, check=True, stdout=sys.stdout, stderr=sys.stderr)
    except subprocess.CalledProcessError as e:
        sys.exit(e.returncode)
    except FileNotFoundError as e:
        print(f"Error: Could not find file '{e.filename}'", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        try:
            sys.exit(130)
        except SystemExit:
            os._exit(130)

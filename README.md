# Aster

Aster is a hobby x86-64 kernel written in C.

## Features

- x86_64 only, but planning to port to RISC architectures
- Interrupts delivered by the modern APIC
- Page frame bitmap allocator
- VMM paging and pagemap support
- Kernel heap
- Kernel panic, assertions and ELF symbol parsing for helpful stack tracing
- ACPI
- Hardware timers, such as the LAPIC timer, HPET or PIT
- SMP
- Preemptive robin robin scheduler
- Processes and threads
- USTAR initial ramdisk
- ELF loader
- Userspace
- System calls

## Building

Run `make all` in the project root to create a bootable ISO image.

Note that the following dependencies are required:
`make`, `git`, `curl`, `xorriso`, `gcc`, `ld`.

## Running

A helpful Python script is provided. Run `./qemu-runner.py --smp 4` to get started.

To see all the available arguments, run `./qemu-runner.py --help`.

Note that `qemu-system-x86_64` is required.

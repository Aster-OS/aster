#!/usr/bin/env bash
set -euo pipefail
make && ./qemu-runner.py "$@"

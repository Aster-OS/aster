#!/usr/bin/env bash

set -euo pipefail

bear --append --output kernel/compile_commands.json -- make all -j$(nproc)

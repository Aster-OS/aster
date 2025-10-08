#!/usr/bin/env bash

set -euo pipefail

bear --output kernel/compile_commands.json -- make

#!/usr/bin/env bash

set -euo pipefail

./build.sh
./run.sh "$@"

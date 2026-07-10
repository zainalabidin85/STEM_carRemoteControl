#!/usr/bin/env bash
# Flash the built RC firmware onto an ESP32 DevKit-style board.
# Usage: ./flash.sh [serial-port]
#   e.g. ./flash.sh /dev/ttyUSB0
# If no port is given, esptool will try to auto-detect one.
set -euo pipefail

PORT="${1:-}"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

ARGS=(--chip esp32 --baud 460800)
if [[ -n "$PORT" ]]; then
  ARGS+=(--port "$PORT")
fi

esptool "${ARGS[@]}" write_flash -z 0x0 "$DIR/merged.bin"

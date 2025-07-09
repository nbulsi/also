#!/bin/bash

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <input_verilog_or_blif_file> <output_directory>"
  exit 1
fi

INPUT_FILE="$1"
OUTPUT_DIR="$2"

BASENAME="$(basename "$INPUT_FILE")"
BASENAME_NO_EXT="${BASENAME%.*}"
OUTPUT_AIG="$OUTPUT_DIR/$BASENAME_NO_EXT.aig"

EXT="${INPUT_FILE##*.}"
if [[ "$EXT" != "v" && "$EXT" != "blif" ]]; then
  echo "Error: Unsupported input format. Please provide a .v or .blif file."
  exit 1
fi

if [[ ! -f "$INPUT_FILE" ]]; then
  echo "Error: Input file does not exist."
  exit 1
fi

if [[ ! -d "$OUTPUT_DIR" ]]; then
  echo "Error: Output directory does not exist."
  exit 1
fi

rm -f "$OUTPUT_AIG"

echo "[INFO] Trying ABC..."
abc -c "read $INPUT_FILE; strash; write_aiger $OUTPUT_AIG"

if [[ -f "$OUTPUT_AIG" ]]; then
  echo "[SUCCESS] AIG generated using ABC: $OUTPUT_AIG"
  exit 0
fi

echo "[WARN] ABC failed. Trying Yosys..."

case "$EXT" in
  v)
    yosys -q -p "read_verilog $INPUT_FILE; hierarchy -auto-top; proc; opt;  techmap; abc -g AND; write_aiger $OUTPUT_AIG"
    ;;
  blif)
    yosys -q -p "read_blif $INPUT_FILE; hierarchy -auto-top; proc; opt;  techmap; abc -g AND; write_aiger $OUTPUT_AIG"
    ;;
esac

if [[ -f "$OUTPUT_AIG" ]]; then
  echo "[SUCCESS] AIG generated using Yosys fallback: $OUTPUT_AIG"
  exit 0
else
  echo "[ERROR] Failed to generate AIG from $INPUT_FILE"
  exit 1
fi

#!/bin/bash

# Name of the C++ file and output binary
SRC_FILE="interconnection.cpp"
OUT_BIN="interconnection"

# Additional source files
EXTRA_SRC="dgraph_logger.cpp"

# Default output image name
PNG_NAME=${1:-network.png}  # Use first argument, or default to "network.png"

# Compile the C++ code
echo "[*] Compiling $SRC_FILE..."
g++ -std=c++17 -o $OUT_BIN $SRC_FILE $EXTRA_SRC
if [ $? -ne 0 ]; then
    echo "[!] Compilation failed!"
    exit 1
fi

# Run the simulation
echo "[*] Running simulation..."
./$OUT_BIN

# Generate PNG from DOT file
if [ -f "network.dot" ]; then
    echo "[*] Generating PNG from DOT..."
    dot -Tpng network.dot -o "$PNG_NAME"
    echo "[✓] network.png generated!"
else
    echo "[!] network.dot not found!"
    exit 1
fi
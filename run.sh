#!/bin/bash

# Name of the C++ file and output binary
SRC_FILE="main.cpp"
OUT_BIN="interconnection"

# Additional source files
LOGGER="dgraph_logger.cpp"
COMPONENT="components.cpp"
LAYER="layers.cpp"

# Default output image name
PNG_NAME=${1:-network.png}  # Use first argument, or default to "network.png"

# Compile the C++ code
echo "[*] Compiling $SRC_FILE..."
g++ -std=c++17 -o $OUT_BIN $SRC_FILE $LAYER $COMPONENT $LOGGER
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
    START=$(date +%s.%N)
    dot -Tpng network.dot -o "$PNG_NAME"
    END=$(date +%s.%N)
    ELAPSED=$(echo "$END - $START" | bc)
    ELAPSED=$(printf "%.9f" "$ELAPSED")
    echo "Graph generation took $ELAPSED seconds" >> report.txt
    echo "[✓] network.png generated!"
else
    echo "[!] network.dot not found!"
    exit 1
fi
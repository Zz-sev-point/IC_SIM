#!/bin/bash

# Name of the C++ file and output binary
SRC_FILE="./src/main.cpp"
OUT_BIN="main"

# Additional source files
LOGGER="./src/dgraph_logger.cpp"
COMPONENT="./src/components.cpp"
LAYER="./src/layers.cpp"
MODEL="./src/model.cpp"

# Default output image name
PNG_NAME=${1:-network.png}  # Use first argument, or default to "network.png"

for bit in 2; do
    for size in 256; do
        # Compile the C++ code
        echo "[*] Compiling $SRC_FILE..."
        g++ -DCROSSBAR_SIZE=$size -DBIT_PRECISION=$bit -std=c++17 -o $OUT_BIN $SRC_FILE $MODEL $LAYER $COMPONENT $LOGGER
        if [ $? -ne 0 ]; then
            echo "[!] Compilation failed!"
            exit 1
        fi

        # Run the simulation
        echo "[*] Running simulation..."
        ./$OUT_BIN
    done
done
#  

Generate PNG from DOT file
if [ -f "network.dot" ]; then
    echo "[*] Generating PNG from DOT..."
    START=$(date +%s.%N)
    dot -Tpng network.dot -o "$PNG_NAME"
    END=$(date +%s.%N)
    ELAPSED=$(echo "$END - $START" | bc)
    ELAPSED=$(printf "%.9f" "$ELAPSED")
    # echo "Graph generation took $ELAPSED seconds" >> report.txt
    echo "[✓] network.png generated!"
else
    echo "[!] network.dot not found!"
    exit 1
fi
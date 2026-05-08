#!/bin/bash

# Ensure the script has execution permission on itself (auto-correction)
chmod +x "$0" 2>/dev/null

# Enter the source folder. If the folder does not exist, the script stops here (-e)
cd source || { echo "Error: 'source' folder not found!"; exit 1; }

echo "Compiling ARCAPI..."

gcc core.c -o core -pthread

echo "Moving binaries to the main folder..."

mv core ../core

chmod +x core 

echo "--------------------------------"
echo "ARCAPI Compilation Completed!"
echo "Run: sudo ./core"
echo "--------------------------------"

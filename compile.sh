#!/bin/bash

# Ensure the script has execution permission on itself
chmod +x "$0" 2>/dev/null

echo "Compiling ALRI CWB Ecosystem..."

# 1. Compile Core Orchestrator
echo "Building AR-CORE..."
gcc ar-core/main.c -o core -pthread -w

# 2. Compile Server/API Module using Makefile
echo "Building AR-WS (WebServices) & AR-BEMF (Micro Framework)..."
make -C ar-ws

chmod +x core arc_server

echo "--------------------------------"
echo "ALRI CWB Compilation Completed!"
echo "Run: sudo ./core"
echo "--------------------------------"

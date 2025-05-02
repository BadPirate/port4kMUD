#!/bin/bash
# Launch script for Port4kMUD - Handles both MUD server and web interface startup
set -e

echo "===== Starting Port4kMUD Launch Script ====="

# Build and start the MUD server
echo "===== Building MUD Server ====="
cd mud
./configure
cd src
make
cd ..

echo "===== Starting MUD Server ====="
# Create log directory if it doesn't exist
mkdir -p log

# Start the MUD server and redirect output to log file
bin/circle > log/circle.log 2>&1 &
MUD_PID=$!
echo "MUD server started with PID: $MUD_PID"

# Give the MUD server a moment to initialize
sleep 5

# Start log streaming process in the background
echo "===== Starting Log Streaming ====="
tail -f log/circle.log log/syslog* &
LOG_PID=$!
echo "Log streaming started with PID: $LOG_PID"

# Build and start the web application
echo "===== Setting up Web Interface ====="
cd ../server
yarn install
yarn build

echo "===== Starting Web Interface ====="
# Start the web app, which will be the primary process
exec yarn start
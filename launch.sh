#!/bin/bash
# Launch script for Port4kMUD - Handles both MUD server and web interface startup
set -e

echo "===== Starting Port4kMUD Launch Script ====="

echo "===== Starting MUD Server ====="
# Create log directory if it doesn't exist
mkdir -p mud/log

# Start the MUD server and redirect output to log file
cd mud
bin/circle > log/circle.log 2>&1 &
MUD_PID=$!
echo "MUD server started with PID: $MUD_PID"
cd ..

# Give the MUD server a moment to initialize
sleep 5

# Start log streaming process in the background
echo "===== Starting Log Streaming ====="
tail -f mud/log/circle.log mud/log/syslog* &
LOG_PID=$!
echo "Log streaming started with PID: $LOG_PID"

echo "===== Starting Web Interface ====="
# Start the web app, which will be the primary process
cd server
# Use the PORT environment variable provided by Dokku, default to 3000 if not set
export PORT=${PORT:-3000}
echo "Starting web interface on port $PORT"
exec yarn start
#!/bin/bash
# Test-specific launch script for Port4kMUD server
# This version only starts the MUD server without the web interface

set -e

echo "===== Starting MUD Server for Testing ====="

# Go to the mud directory
cd ../mud

# Configure and build if needed
if [ ! -f "bin/circle" ]; then
  echo "===== Building MUD Server ====="
  ./configure
  cd src
  make
  cd ..
fi

# Create log directory if it doesn't exist
mkdir -p log

# Start the MUD server and redirect output to log file
bin/circle -q > log/test-circle.log 2>&1 &
MUD_PID=$!
echo "MUD server started with PID: $MUD_PID"
echo $MUD_PID > ../server/e2e/mud_server.pid

# Give the MUD server a moment to initialize
sleep 3

echo "===== MUD Server Ready for Testing ====="
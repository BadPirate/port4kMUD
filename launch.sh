#!/bin/bash
# Launch script for Port4kMUD - Handles both MUD server and web interface startup
set -e

echo "===== Starting Port4kMUD Launch Script ====="

# Create required directories for MUD operation
echo "===== Setting up MUD directories ====="
mkdir -p mud/log
mkdir -p mud/lib/text
mkdir -p mud/lib/world/zon

# Copy initial data files if they don't exist
if [ ! -f mud/lib/text/motd ]; then
  echo "Initializing MUD data files..."
  # Copy from the repo's initial data if it exists
  if [ -d mud/lib-dist ]; then
    cp -r mud/lib-dist/* mud/lib/
  fi
fi

echo "===== Starting MUD Server ====="
# Start the MUD server and redirect output to log file
cd mud
# Run the MUD on port 4000 as CircleMUD expects
bin/circle 4000 > >(sed 's/^/MUD: /') 2> >(sed 's/^/MUDERR: /' >&2) &
MUD_PID=$!
echo "MUD server started with PID: $MUD_PID"
cd ..

# Give the MUD server a moment to initialize
sleep 5

echo "===== Starting Web Interface ====="
# Start the web app, which will be the primary process
cd server
# Force web server to use port 3000 regardless of what Dokku sets
# This ensures no conflict with the MUD server running on port 4000
unset PORT
export PORT=3000
echo "Starting web interface on port $PORT"
exec yarn start
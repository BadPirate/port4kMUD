#!/bin/bash
# Test cleanup script to terminate the MUD server after testing

# Get the directory of the script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PID_FILE="${SCRIPT_DIR}/mud_server.pid"

# Check if the PID file exists
if [ -f "$PID_FILE" ]; then
  # Read the PID
  MUD_PID=$(cat "$PID_FILE")
  
  echo "===== Cleaning up MUD Server (PID: $MUD_PID) ====="
  
  # Check if the process is still running
  if kill -0 $MUD_PID 2>/dev/null; then
    # Kill the MUD server process
    kill $MUD_PID
    echo "MUD server process terminated"
  else
    echo "MUD server process already ended"
  fi
  
  # Remove the PID file
  rm "$PID_FILE"
else
  echo "No MUD server PID file found at $PID_FILE, nothing to clean up"
fi

echo "===== Cleanup Complete ====="
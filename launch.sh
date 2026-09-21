#!/bin/bash
# Launch script for Port4kMUD - Handles both MUD server and web interface startup
set -e

echo "===== Starting Port4kMUD Launch Script ====="

# Create required directories for MUD operation
echo "===== Setting up MUD directories ====="
mkdir -p mud/log
mkdir -p mud/lib/text
mkdir -p mud/lib/world/zon
# The web portal's accounts database lives here too, on the same persistent
# volume, so accounts survive a redeploy (see COOLIFY.md).
mkdir -p mud/lib/etc

# Seed the persistent volume the first time it comes up empty. The image ships
# the initial game data as mud/lib-dist rather than mud/lib, because a copy at
# mud/lib would be shadowed by the volume mounted over it.
if [ ! -f mud/lib/text/motd ]; then
  echo "Initializing MUD data files..."
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
echo "===== Applying portal database migrations ====="
# Called through node_modules/.bin rather than `yarn prisma:migrate` so the
# production image needs nothing from yarn at boot.
./node_modules/.bin/prisma migrate deploy
echo "Starting web interface on port $PORT"
# exec'd directly rather than through `yarn start` so that node is PID 1 and
# receives the SIGTERM a redeploy sends instead of yarn swallowing it.
# NODE_ENV=production is set in the image.
exec node dist/server.js

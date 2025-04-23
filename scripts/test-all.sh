#!/usr/bin/env bash
set -euo pipefail

# Tear down any existing test containers
docker compose -f docker-compose.test.yml down -v || true

# Start test services
docker compose -f docker-compose.test.yml up -d db mailhog test-server

# Wait for the Next.js server to accept connections
npx wait-on tcp:localhost:3000

# Run Playwright tests
npx playwright test --forbid-only --max-failures=1

# Tear down test services
docker compose -f docker-compose.test.yml down -v
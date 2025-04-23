
#!/usr/bin/env bash
set -euo pipefail

# Tear down any existing test containers
docker compose -f docker-compose.test.yml down -v || true

# Export test environment variables
export DATABASE_URL="postgresql://postgres:postgres@localhost:5432/nextstrap_test?schema=public"
export SMTP_HOST="localhost"
export SMTP_PORT="1025"
export SMTP_USER=""
export SMTP_PASS=""
export EMAIL_FROM="nextstrap@example.com"
export NEXTAUTH_URL="http://localhost:3000"
export TEST_MODE="true"
export NEXT_PUBLIC_TEST_MODE="true"
export NEXT_PUBLIC_APP_NAME="nextstrap"
export NEXTAUTH_SECRET="test_secret"

# Start test services
docker compose -f docker-compose.test.yml up -d db mailhog test-server

# Wait for the Next.js server to accept connections
npx wait-on tcp:localhost:3000

# Run Playwright tests
npx playwright test --forbid-only --max-failures=1

# Tear down test services
docker compose -f docker-compose.test.yml down -v

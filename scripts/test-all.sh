
#!/usr/bin/env bash
set -euo pipefail

# Export test environment variables
export DATABASE_URL="file:./test.db"
export SMTP_HOST="0.0.0.0"
export SMTP_PORT="1025"
export SMTP_USER=""
export SMTP_PASS=""
export EMAIL_FROM="nextstrap@example.com"
export NEXTAUTH_URL="http://0.0.0.0:3000"
export TEST_MODE="true"
export NEXT_PUBLIC_TEST_MODE="true"
export NEXT_PUBLIC_APP_NAME="nextstrap"
export NEXTAUTH_SECRET="test_secret"

# Generate Prisma client
npx prisma generate --schema=prisma/schema.sqlite.prisma

# Push schema to SQLite database
npx prisma db push --schema=prisma/schema.sqlite.prisma --force-reset

# Start the dev server
npx next dev -p 3000 & 
SERVER_PID=$!

# Wait for the server
npx wait-on tcp:3000

# Run Playwright tests
npx playwright test --forbid-only --max-failures=1

# Cleanup
kill $SERVER_PID
rm -f test.db

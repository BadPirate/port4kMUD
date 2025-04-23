
const { execSync } = require('child_process')
const fs = require('fs')

// Set environment variables
process.env.DATABASE_URL = 'file:./test.db'
process.env.SMTP_HOST = '0.0.0.0'
process.env.SMTP_PORT = '1025'
process.env.EMAIL_FROM = 'test@example.com'
process.env.NEXTAUTH_URL = 'http://0.0.0.0:3000'
process.env.NEXTAUTH_SECRET = 'test_secret'

// Remove existing test database
try {
  fs.unlinkSync('test.db')
  fs.unlinkSync('test.db-journal')
} catch (err) {
  // Ignore if files don't exist
}

// Generate Prisma client and push schema
execSync('npx prisma generate --schema=prisma/schema.sqlite.prisma')
execSync('npx prisma db push --schema=prisma/schema.sqlite.prisma --force-reset')

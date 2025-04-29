#!/usr/bin/env node
const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

// Export an async function as required by Jest globalSetup
module.exports = async () => {
  // Define the test database path
  const dbPath = 'test.db';
  const absDbPath = path.resolve(process.cwd(), dbPath);
  const databaseUrl = 'file:' + absDbPath;

  // Set environment variables for the test run
  process.env.DATABASE_URL = databaseUrl;
  // Add other necessary test environment variables if needed
  // process.env.SMTP_HOST = '0.0.0.0';
  // process.env.SMTP_PORT = '1025';
  // process.env.EMAIL_FROM = 'test@example.com';
  // process.env.NEXTAUTH_URL = 'http://0.0.0.0:3000';
  // process.env.NEXTAUTH_SECRET = 'test_secret';

  console.log(`Setting up test database: ${dbPath}`);

  // --- Add safety check ---
  if (!absDbPath.endsWith('test.db')) {
    console.error(`Safety check failed: Attempting to modify non-test database path: ${absDbPath}`);
    process.exit(1);
  }
  // --- End safety check ---

  // Remove existing test database files
  try {
    if (fs.existsSync(absDbPath)) {
      fs.unlinkSync(absDbPath);
      console.log(`Removed existing test database file: ${dbPath}`);
    }
    const journalPath = absDbPath + '-journal';
    if (fs.existsSync(journalPath)) {
      fs.unlinkSync(journalPath);
      console.log(`Removed existing test database journal file: ${journalPath}`);
    }
  } catch (err) {
    console.error('Error removing existing test database files:', err);
    // Decide if you want to exit or continue
  }

  // Determine the correct schema path (assuming SQLite for tests)
  const provider = 'sqlite';
  const schemaPath = path.resolve(
    process.cwd(),
    'prisma',
    'generated',
    `${provider}.prisma`,
  );

  if (!fs.existsSync(schemaPath)) {
    console.error(`Error: Prisma schema not found at ${schemaPath}`);
    console.log('Please run `yarn build:prisma-schemas` first.');
    process.exit(1);
  }

  // Prepare environment for Prisma CLI
  const env = { ...process.env, DATABASE_URL: databaseUrl };

  try {
    // Push the schema to the database
    console.log(`Pushing Prisma schema from ${schemaPath} to ${databaseUrl}...`);
    // Use --accept-data-loss for non-interactive environments like CI/tests
    execSync(`npx prisma db push --schema=${schemaPath} --force-reset --accept-data-loss`, { stdio: 'inherit', env });
    console.log('Database schema pushed successfully.');

    // Generate Prisma client for the test database
    console.log(`Generating Prisma client for ${provider}...`);
    execSync(`npx prisma generate --schema=${schemaPath}`, { stdio: 'inherit', env });
    console.log('Prisma client generated successfully.');

  } catch (error) {
    console.error('Failed to set up test database:', error);
    process.exit(1);
  }

  console.log('Test database setup complete.');
};

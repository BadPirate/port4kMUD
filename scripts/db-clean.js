#!/usr/bin/env node
const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

// Determine database URL (defaults to local SQLite dev.db)
let databaseUrl = process.env.DATABASE_URL || 'file:./dev.db';
// If using SQLite (file:), convert to absolute file URL and delete existing file
let finalDatabaseUrl = databaseUrl;
if (databaseUrl.startsWith('file:')) {
  // Strip leading 'file:' and optional '//' to get the filesystem path
  const dbPath = databaseUrl.replace(/^file:(?:\/\/)?/, '');
  const absPath = path.resolve(process.cwd(), dbPath);
  // Remove existing file if present
  if (fs.existsSync(absPath)) {
    fs.unlinkSync(absPath);
    console.log(`Removed existing SQLite database file: ${dbPath}`);
  }
  // Use absolute file URL so that CLI resolves correctly
  finalDatabaseUrl = 'file:' + absPath;
} else {
  console.log('DATABASE_URL is not a SQLite file URL; skipping file deletion.');
}

// If using SQLite (file:), delete the existing database file
if (databaseUrl.startsWith('file:')) {
  // Strip leading 'file:' and optional '//' to get the filesystem path
  const dbPath = databaseUrl.replace(/^file:(?:\/\/)?/, '');
  const fullPath = path.resolve(process.cwd(), dbPath);
  if (fs.existsSync(fullPath)) {
    fs.unlinkSync(fullPath);
    console.log(`Removed existing SQLite database file: ${dbPath}`);
  }
} else {
  console.log('DATABASE_URL is not a SQLite file URL; skipping file deletion.');
}

// Determine provider from URL prefix
const provider =
  finalDatabaseUrl.startsWith('postgresql://') || finalDatabaseUrl.startsWith('postgres://')
    ? 'postgresql'
    : 'sqlite';
// Locate the corresponding generated schema file
const schemaPath = path.resolve(
  process.cwd(),
  'prisma',
  'generated',
  `${provider}.prisma`,
);
// Prepare environment for Prisma CLI
const env = Object.assign({}, process.env, { DATABASE_URL: finalDatabaseUrl });

console.log(`Pushing Prisma schema to create ${provider} dev database from ${schemaPath}...`);
execSync(`npx prisma db push --schema=${schemaPath}`, { stdio: 'inherit', env });
console.log('Database schema pushed');

console.log(`Generating Prisma client for ${provider}...`);
execSync(`npx prisma generate --schema=${schemaPath}`, { stdio: 'inherit', env });
console.log('Prisma client generated');
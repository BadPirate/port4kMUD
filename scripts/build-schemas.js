#!/usr/bin/env node
const fs = require('fs')
const path = require('path')

// Directories
const prismaDir = path.resolve(__dirname, '..', 'prisma')
const wrappersDir = path.resolve(prismaDir, 'wrappers')
const modelsPath = path.resolve(prismaDir, 'models.prisma')
const generatedDir = path.resolve(prismaDir, 'generated')

// Ensure generated directory exists
if (!fs.existsSync(generatedDir)) {
  fs.mkdirSync(generatedDir, { recursive: true })
}

// Read models
const models = fs.readFileSync(modelsPath, 'utf-8')

// Providers to generate
const providers = [
  { name: 'sqlite', wrapper: 'sqlite.prisma' },
  { name: 'postgresql', wrapper: 'postgresql.prisma' },
]

providers.forEach(({ name, wrapper }) => {
  const wrapperPath = path.resolve(wrappersDir, wrapper)
  const wrapperContent = fs.readFileSync(wrapperPath, 'utf-8')

  // --- Inject output path into generator block (Revised) ---
  const generatorRegex = /(generator\s+client\s+\{)([^}]*)(\s*\})/; // Capture parts: opening, content, closing
  const modifiedWrapperContent = wrapperContent.replace(
    generatorRegex,
    (match, opening, content, closing) => {
      if (content.includes('output')) {
        return match; // Already has output, do nothing
      }
      // Add output path, ensuring proper indentation and newline before closing brace
      const indentedOutput = '\n  output   = "../node_modules/@prisma/client"';
      // Ensure content ends with a newline if it's not empty and trim existing whitespace
      const formattedContent = content.trim() + (content.trim() ? '\n' : '');
      return opening + '\n' + formattedContent + indentedOutput + '\n' + closing.trim(); // Reconstruct with newlines
    }
  );
  // -------------------------------------------------------

  const outputContent = modifiedWrapperContent + '\n\n' + models
  const outPath = path.resolve(generatedDir, `${name}.prisma`)
  fs.writeFileSync(outPath, outputContent)
  console.log(`Generated ${path.relative(process.cwd(), outPath)}`)
})

// Note: schema.prisma is left as a stub; at runtime and in scripts we pass --schema explicitly
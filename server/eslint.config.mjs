import { fileURLToPath } from 'node:url'
import path from 'node:path'
import globals from 'globals'
import nextConfig from 'eslint-config-next'
import importPlugin from 'eslint-plugin-import'
import prettierRecommended from 'eslint-plugin-prettier/recommended'
import reactPlugin from 'eslint-plugin-react'
import { parser as tsParser, plugin as tsPlugin } from 'typescript-eslint'

const tsconfigRootDir = path.dirname(fileURLToPath(import.meta.url))

const config = [
  {
    // Nothing here is ours to lint: build output, vendored assets and the
    // Prisma client generated from prisma/schema.prisma.
    ignores: [
      '.next/**',
      'out/**',
      'build/**',
      'dist/**',
      'public/**',
      'src/generated/**',
      'next-env.d.ts',
      'styles/bootstrap.min.css',
      'jest.config.js',
      'jest.setup.js',
    ],
  },

  ...nextConfig,
  prettierRecommended,

  {
    // Rules that apply everywhere. Plugins are named explicitly because flat
    // config resolves a rule's plugin only from config objects matching the
    // same file - eslint-config-next scopes its own to .ts/.tsx.
    files: ['**/*.{js,jsx,mjs,ts,tsx}'],
    plugins: { react: reactPlugin, import: importPlugin },
    rules: {
      'react/jsx-filename-extension': [2, { extensions: ['.js', '.jsx', '.ts', '.tsx'] }],
      'react/function-component-definition': [
        2,
        { namedComponents: 'arrow-function', unnamedComponents: 'arrow-function' },
      ],
      'react/jsx-props-no-spreading': 'off',
      'react/jsx-one-expression-per-line': 'off',
      'react/prop-types': 'off',

      'import/extensions': ['error', 'never'],
      'import/no-cycle': 'warn',
      'import/named': 'warn',
      'import/no-unresolved': 'off',
      'import/no-named-as-default': 'warn',
      'import/no-named-as-default-member': 'warn',
      'import/order': 'warn',

      'no-unused-vars': 'off',

      // Forbid direct usage of process.env - src/utils/config.ts is the one
      // place that reads it (see the override below).
      'no-restricted-syntax': [
        'error',
        {
          selector: 'MemberExpression[object.name="process"][property.name="env"]',
          message: 'Direct use of process.env is forbidden. Use the config utility instead.',
        },
      ],

      'no-console': 'warn',
      'no-param-reassign': 'warn',
      'no-useless-escape': 'warn',
      'no-multiple-empty-lines': 'warn',
      'eol-last': 'warn',
      'react-hooks/exhaustive-deps': 'warn',
      'prettier/prettier': 'warn',
    },
  },

  {
    // Type-aware rules, and the type-aware parser settings they need.
    files: ['**/*.{ts,tsx}'],
    plugins: { '@typescript-eslint': tsPlugin },
    languageOptions: {
      parser: tsParser,
      parserOptions: {
        project: './tsconfig.json',
        tsconfigRootDir,
        ecmaFeatures: { jsx: true },
      },
    },
    rules: {
      '@typescript-eslint/no-unused-vars': [
        'error',
        {
          argsIgnorePattern: '^_',
          varsIgnorePattern: '^_',
          caughtErrorsIgnorePattern: '^_',
        },
      ],
      '@typescript-eslint/no-explicit-any': 'warn',
      '@typescript-eslint/return-await': 'warn',
      'no-return-await': 'off',
      // The server, the bridge and the Discord relay log through console;
      // this matches the previous .eslintrc.js override.
      'no-console': 'off',
    },
  },

  {
    // The centralized config utility, and the config files that must read the
    // environment before it is loadable, are the exceptions to the ban above.
    files: ['src/utils/config.ts', 'prisma.config.ts', 'eslint.config.mjs', 'jest.config.js'],
    rules: { 'no-restricted-syntax': 'off' },
  },

  {
    files: ['__tests__/**/*', 'e2e/**/*', '**/*.test.{js,jsx,ts,tsx}'],
    rules: { 'no-console': 'off' },
  },

  {
    // Config files are plain scripts, not app sources.
    files: ['*.js', '*.mjs'],
    languageOptions: { globals: { ...globals.node } },
  },
]

export default config

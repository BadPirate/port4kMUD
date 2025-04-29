module.exports = {
  parser: '@typescript-eslint/parser',
  parserOptions: {
    project: './tsconfig.json',
    tsconfigRootDir: __dirname,
    ecmaVersion: 2020,
    sourceType: 'module',
    ecmaFeatures: {
      jsx: true,
    },
  },
  plugins: ['@typescript-eslint', 'react', 'import'],
  extends: [
    'next/core-web-vitals',
    'plugin:react/recommended',
    'plugin:@typescript-eslint/recommended',
    'plugin:prettier/recommended',
  ],
  env: {
    browser: true,
    es6: true,
    node: true,
  },
  settings: {
    'import/resolver': {
      node: {
        extensions: ['.js', '.jsx', '.ts', '.tsx'],
        moduleDirectory: ['node_modules', 'src/'],
      },
    },
  },
  rules: {
    semi: ['error', 'never'],
    indent: ['error', 2],
    // Re-enable undefined variable checks with TypeScript handling them
    'no-undef': 'error',
    'react/jsx-filename-extension': [2, { extensions: ['.js', '.jsx', '.ts', '.tsx'] }],
    'react/function-component-definition': [
      2,
      { namedComponents: 'arrow-function', unnamedComponents: 'arrow-function' },
    ],
    'react/react-in-jsx-scope': 'off', // This is fine for Next.js which doesn't need React imports
    'import/extensions': ['error', 'never'],
    'react/jsx-props-no-spreading': 'off', // This is a stylistic choice that doesn't indicate bugs
    'react/jsx-one-expression-per-line': 'off', // This is a stylistic choice handled by Prettier
    // Re-enable import cycle detection which can cause real issues
    'import/no-cycle': 'warn',
    'import/named': 'warn',
    // Keep this off as TypeScript handles it better with path aliases
    'import/no-unresolved': 'off',
    'import/no-named-as-default': 'warn',
    'import/no-named-as-default-member': 'warn',
    // Configure the no-unused-vars rule to ignore variables starting with _
    'no-unused-vars': 'off', // Turn off base rule as it can report incorrect errors
    '@typescript-eslint/no-unused-vars': [
      'error',
      {
        argsIgnorePattern: '^_',
        varsIgnorePattern: '^_',
        caughtErrorsIgnorePattern: '^_',
      },
    ],
  },
  overrides: [
    {
      files: ['**/*.ts', '**/*.tsx'],
      rules: {
        'react/prop-types': 'off', // Not needed with TypeScript
        'import/order': 'warn', // Promote organized imports

        // Formatting rules handled by Prettier can remain off
        semi: 'off',
        '@typescript-eslint/semi': 'off',
        quotes: 'off',
        '@typescript-eslint/quotes': 'off',
        indent: 'off',
        '@typescript-eslint/indent': 'off',
        'comma-dangle': 'off',
        '@typescript-eslint/comma-dangle': 'off',
        'object-curly-newline': 'off',
        '@typescript-eslint/object-curly-spacing': 'off',
        'object-curly-spacing': 'off',
        'function-paren-newline': 'off',

        // Re-enable rules that catch actual bugs or bad patterns
        '@typescript-eslint/return-await': 'warn',
        'no-return-await': 'off', // Disabled in favor of TypeScript version
        'eol-last': 'warn',
        'no-param-reassign': 'warn',
        // Allow console for development but consider warning
        'no-console': process.env.NODE_ENV === 'production' ? 'warn' : 'off',
        // These catch important TypeScript issues
        '@typescript-eslint/no-explicit-any': 'warn',

        'no-multiple-empty-lines': 'warn',

        // Keep formatting rules off since Prettier handles them
        'max-len': 'off',
        'react/jsx-wrap-multilines': 'off',
        'react/jsx-indent': 'off',
        'react/jsx-closing-tag-location': 'off',
        'react/jsx-indent-props': 'off',

        // Re-enable this as it catches genuine errors
        'no-useless-escape': 'warn',
      },
    },
    {
      // Add Jest environment for test files
      files: ['**/__tests__/**/*', '**/*.test.{js,jsx,ts,tsx}'],
      env: {
        jest: true, // This makes Jest globals available
      },
      rules: {
        'no-console': 'off',
      },
    },
    {
      // Allow more console usage in test files
      files: ['**/__tests__/**/*', '**/e2e/**/*'],
      rules: {
        'no-console': 'off',
      },
    },
    {
      // Special rules for database-related files
      files: ['**/db.ts', '**/prisma/**/*'],
      rules: {
        // Completely turn off camelcase for database files
        camelcase: 'off',
        '@typescript-eslint/camelcase': 'off',
      },
    },
  ],
}

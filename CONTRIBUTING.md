# Contributing to Nextstrap

Thank you for your interest in contributing to Nextstrap! Please read the following guidelines.

## Code of Conduct

This project follows the [Contributor Covenant Code of Conduct](https://www.contributor-covenant.org/). Please be respectful and inclusive.

## Getting Started

1. Fork the repository.
2. Clone your fork: `git clone git@github.com:<your-username>/nextstrap.git`
3. Navigate to the project directory: `cd nextstrap`
4. Install dependencies: `yarn install`
5. Generate Prisma schemas: `yarn build:prisma-schemas`
6. Set up your development database: `yarn build:dev-db`

**Note**: This project uses the Yarn package manager, not npm.

7. Start the development server: `yarn dev`

## Development Guidelines

- **Frameworks & Languages**: Next.js with TypeScript.
- **Linting**: ESLint configured with Airbnb, Airbnb TypeScript, and Next.js rules.
  - Run `yarn lint` and fix any linting errors.
  - Note: this project uses no semicolons (as per the style guide).
- **Formatting**: We adhere to the ESLint style. Avoid introducing trailing semicolons.
- **Type Safety**: TypeScript is configured in strict mode. Ensure no type errors.
- **File Structure**:
  - `pages/` for Next.js pages.
  - `src/` for additional components and utilities.
  - `styles/` for global CSS (Bootstrap theme).
  - `public/` for static assets.
- **Database Changes**:
  - Edit models in `prisma/models.prisma`
  - Run `yarn build:prisma-schemas` to regenerate schemas
  - Run `yarn build:dev-db` to update your local database
- **Testing**:
  - Unit tests: `yarn test`
  - End-to-end tests: `yarn test:e2e`
  - See README.md for more details on testing.

## Commit Messages

Use [Conventional Commits](https://www.conventionalcommits.org/):

- `feat: add new feature`
- `fix: fix a bug`
- `docs: update documentation`
- `style: code style changes (no semantic impact)`
- `refactor: code changes that neither fix a bug nor add a feature`
  Include issue references: `feat(api): add user endpoint (#123)`

## Branching & Pull Requests

- Create branches from `main`:
  - Feature branches: `feature/<short-description>`
  - Bugfix branches: `fix/<short-description>`
- Open a pull request against `main` with a clear title and description.
- Ensure all checks pass (lint, type checks, etc.).
- Request reviews from at least one maintainer.

## License

This project is private and not published. Contributions are subject to the project's license.

# Nextstrap

A simple project the way Badpirate likes it:

- Next JS
- Typescript
- Eslint (Airbnb)
- No forking semi-colons

Feel free to fork for your own project if want to make a quick web project based on the best setup available.

## Features

- **Prisma ORM**: Manage databases with Prisma, supporting SQLite and PostgreSQL.
- **Playwright for End-to-End Testing**: Browser-based testing with debugging and headless options.
- **Jest for Unit Testing**: Comprehensive unit testing setup with mocking and testing-library support.
- **Bootstrap and Bootswatch Themes**: Preinstalled Cyborg theme with easy customization.
- **Environment Variable Management**: `.env` files for app configuration.
- **Husky for Git Hooks**: Pre-commit hooks for linting and formatting.
- **Prettier for Code Formatting**: Enforces consistent code style.
- **TypeScript Strict Mode**: Ensures type safety with strict mode enabled.
- **Custom Prisma Schema Generation**: Scripts for dynamic schema generation for multiple database providers.

## Using

1. `git clone git@github.com:BadPirate/nextstrap.git MyCoolNextProject`
2. Rename the project in `package.json`
3. `yarn dev`

**Note**: This project uses the Yarn package manager, not npm.

## Theme

Currently comes preinstalled with [Cyborg](https://bootswatch.com/cyborg/), but you can download any theme that you want to use or the default min theme. Replacing the min'd css at `styles/bootstrap.min.css`

## Testing

This project includes an end-to-end test suite powered by [Playwright Test](https://playwright.dev) and unit tests with [Jest](https://jestjs.io/).

- **End-to-End Tests**:

  - Tests are located in the `e2e/` directory, with configuration in `playwright.config.ts`.
  - Browser binaries can be installed (or shared) via:
    ```bash
    npx playwright install
    ```
  - Run the full suite with:
    ```bash
    yarn test:e2e
    ```
    (alias for `npx playwright test`, which will automatically start the local dev server on port 3000)
  - Common flags:
    - `--headed` to view browsers during tests
    - `--debug` to launch the interactive inspector
    - `--project=chromium|firefox|webkit` to target a specific browser

- **Unit Tests**:

  - Unit tests are located in the `__tests__/` directory.
  - Run the unit tests with:
    ```bash
    yarn test
    ```
  - Jest is configured with support for mocking and testing-library utilities.

- **Linting and Type Checking**:
  - Run linting and type checks with:
    ```bash
    yarn lint
    ```

## Prisma Configuration and Setup

This project uses [Prisma](https://www.prisma.io/) as the ORM for database management. It supports both SQLite and PostgreSQL.

### Configuration

- Prisma schemas are located in the `prisma/` directory.

  - `models.prisma`: Core application models.
  - `generated/`: Contains generated schemas for SQLite and PostgreSQL.
  - `wrappers/`: Wrapper schemas for database providers.

- Environment variables for database configuration are managed in the `.env` file.

### Setup

1. Install dependencies:

   ```bash
   yarn install
   ```

2. Generate Prisma client:

   ```bash
   npx prisma generate
   ```

3. Push the database schema:

   ```bash
   npx prisma db push
   ```

4. For testing, ensure the test database is set up:

   ```bash
   node scripts/setup-test-db.js
   ```

5. To clean and rebuild schemas:
   ```bash
   yarn build:prisma-schemas
   ```

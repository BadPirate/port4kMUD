# Nextstrap

A simple project starter template for modern web development.

Feel free to fork for your own project if want to make a quick web project based on the best setup available.

## Features

This project includes a comprehensive set of features designed to maintain high code quality standards, making it ideal for working with AI assistants and ensuring consistent development practices.

### Core Technologies

- **Next.js**: React framework with server-side rendering and routing
- **TypeScript**: With strict mode enabled for maximum type safety
- **Prisma ORM**: Database management supporting both SQLite and PostgreSQL
- **Bootstrap & React Bootstrap**: UI framework with Bootswatch themes

### Development & Quality Tools

- **Strict ESLint & Prettier Integration**: Enforces Airbnb TypeScript and Next.js linting rules
- **No Semicolons Style**: Enforced code style as per project philosophy
- **Environment Variable Management**: `.env` files for app configuration using dotenv
- **Husky & Lint-Staged**: Pre-commit hooks for linting, formatting, and type-checking
- **Modern Project Structure**: Separation of concerns with `src/`, `pages/`, `styles/`, and `prisma/` directories

### Database Features

- **Custom Prisma Schema Generation**: Scripts for dynamic schema generation for multiple database providers
- **Database Utilities**: Scripts for cleaning, rebuilding, and setting up test databases

### Testing Suite

- **Playwright for End-to-End Testing**: Browser-based testing with debugging and headless options
- **Jest for Unit Testing**: Comprehensive testing setup with mocking and testing-library utilities
- **CI/CD Ready**: Scripts and configuration for continuous integration pipelines

### UI & Styling

- **Bootswatch Theme Support**: Preinstalled with Cyborg theme
- **Customizable Bootstrap Theme**: Easily swap themes by replacing a single CSS file

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

2. Generate Prisma schemas for your selected database provider:

   ```bash
   yarn build:prisma-schemas
   ```

3. Set up your development database:

   ```bash
   yarn build:dev-db
   ```

4. For testing, the test database is automatically set up when running tests.

## How to Add Model Elements

1. Edit `prisma/models.prisma` to define or update your data models. For example, to add a new field to the `User` model or create a new model, simply edit this file.
2. After making changes, you must rebuild the generated Prisma schemas for all supported databases:

   ```bash
   yarn build:prisma-schemas
   ```

3. To reset your development database to match the new schema:

   ```bash
   yarn build:dev-db
   ```

## How to Build the Database

To generate the database schema and Prisma client for your current environment:

```bash
yarn build:prisma-schemas
```

## How to Launch a Clean Local SQLite Database for Development

To reset and initialize a fresh local SQLite database for development, run:

```bash
yarn build:dev-db
```

This will:

- Rebuild the Prisma schemas
- Remove any existing local SQLite database file (`dev.db` by default)
- Push the latest schema to the database
- Generate the Prisma client

You can now start your dev server with:

```bash
yarn dev
```

## How to Use This Template for Your Own Project

- Fork or clone this repository.
- Edit `package.json` to update the project name and details.
- Update or replace models in `prisma/models.prisma`.
- Run `yarn build:prisma-schemas` and `yarn build:dev-db` to set up your database.
- Start developing your app!

# Nextstrap

A simple project the way Badpirate likes it:

- Next JS
- Typescript
- Eslint (Airbnb)
- No forking semi-colons

Feel free to fork for your own project if want to make a quick web project based on the best setup available.

## Using

1. `git clone git@github.com:BadPirate/nextstrap.git MyCoolNextProject`
2. Rename the project in `package.json`
3. `yarn dev`

## Theme

Currently comes preinstalled with [Cyborg](https://bootswatch.com/cyborg/), but you can download any theme that you want to use or the default min theme.  Replacing the min'd css at `styles/bootstrap.min.css`

## Testing

This project includes an end-to-end test suite powered by [Playwright Test](https://playwright.dev).

- Tests are located in the `tests/` directory, with configuration in `playwright.config.ts`.
- Browser binaries can be installed (or shared) via:
  ```bash
  npx playwright install
  ```
- Run the full suite with:
  ```bash
  yarn test
  ```
  (alias for `npx playwright test`, which will automatically start the local dev server on port 3000)
- Common flags:
  - `--headed` to view browsers during tests
  - `--debug` to launch the interactive inspector
  - `--project=chromium|firefox|webkit` to target a specific browser
- ESLint is configured to allow devDependencies in test files (via an override in `.eslintrc.js`), so imports from `@playwright/test` won’t trigger extraneous-dependencies errors.
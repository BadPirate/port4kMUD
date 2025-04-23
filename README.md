# Nextstrap

- A simple project the way Badpirate likes it:

- Next.js
- TypeScript
- ESLint (Airbnb)
- React-Bootstrap for UI components
- NextAuth.js for authentication
- No forking semi-colons

Feel free to fork for your own project if want to make a quick web project based on the best setup available.

## Getting Started

1. Clone the repository:

   ```bash
   git clone git@github.com:BadPirate/nextstrap.git MyCoolNextProject
   cd MyCoolNextProject
   ```

2. Rename the `name` field in `package.json` (optional):

   ```json
   // package.json
   {
     "name": "MyCoolNextProject",
     ...
   }
   ```

3. Create a `.env.local` file in the project root by copying the example and customizing as needed:

   ```bash
   cp .env.local.example .env.local
   # Edit .env.local to set your values for:
   # DATABASE_URL, NEXTAUTH_SECRET, NEXTPUBLIC_APP_NAME,
   # SMTP_HOST, SMTP_PORT, EMAIL_FROM, GARAGE_AUTH_CLIENT_ID, etc.
   ```

4. Install dependencies, bring up Docker services, and start the development server:

   ```bash
   yarn install
   yarn db:up      # start Postgres & MailHog via Docker Compose
   yarn dev        # starts Next.js (connects to above services)
   ```
   When finished, you can shut down the services:
   ```bash
   yarn db:down
   ```

## Theme

This project uses React-Bootstrap for UI components and is styled with the [Cyborg](https://bootswatch.com/cyborg/) theme. The compiled CSS is located at `styles/bootstrap.min.css`. To use a different theme, replace this file with your theme's minified CSS.

## Authentication

This branch provides a base [NextAuth.js](https://next-auth.js.org/) with a custom OAuth provider.

- Click the **Login** button in the navigation bar to sign in.
- After signing in, your name is displayed and the button becomes **Logout**.
- The `SessionProvider` in `pages/_app.tsx` wraps your application and provides session context.
- Use the `useSession` hook or the `WithSession` component (`src/components/WithSession.tsx`) to render content based on authentication status.

Configuration for the OAuth provider is located in `pages/api/auth/[...nextauth].tsx`. Update the `issuer`, `clientId`, `clientSecret`, and `wellKnown` URLs as needed.

## Testing

This project includes an end-to-end test suite powered by [Playwright Test](https://playwright.dev).

- Tests are located in the `tests/` directory, with configuration in `playwright.config.ts`.
- Run the full suite with:
  ```bash
  yarn test
  ```
  This will:
  - install Playwright browsers (`npx playwright install`)
  - spin up the app in test mode (SQLite + mock email provider)
  - run all navigation and email-login tests

### System dependencies for Playwright
Before running tests, ensure the required libraries are installed on your system.
On Debian/Ubuntu, for example:
```bash
sudo apt-get update && sudo apt-get install -y \
  libglib2.0-0 libnss3 libnspr4 libatk1.0-0 libatk-bridge2.0-0 \
  libx11-6 libxcomposite1 libxdamage1 libxext6 libxfixes3 libxrandr2 \
  libgbm1 libasound2 libcairo2 libpulse0 libvulkan1
```
Refer to https://playwright.dev/docs/ci#install-dependencies for other platforms.

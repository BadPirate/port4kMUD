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

3. Create a `.env.local` file in the project root with the following environment variables:

   ```
   NEXT_PUBLIC_APP_NAME=MyCoolNextProject
   NEXTAUTH_URL=http://localhost:3000
   GARAGE_AUTH_CLIENT_ID=your_garage_auth_client_id
   GARAGE_AUTH_CLIENT_SECRET=your_garage_auth_client_secret
   ```

4. Install dependencies and start the development server:

   ```bash
   yarn install
   yarn dev
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
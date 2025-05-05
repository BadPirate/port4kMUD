# Co-pilot passdown

For AI Agent "memory", Co-Pilot agents should follow these instructions, and if through conversation and instruction
get new "general" instructions for this project, update the list for future instances. Keep it brief, but put anything
that you learn about the product or the preferred development flow / process below:

---

## Project-Specific Instructions for Port4kMUD

### 1. MUD Server (mud/)
- **Build:**
  1. Run `./configure` in the mud/ directory to generate Makefiles.
  2. Run `cd src && make` to build the main MUD executable (`bin/circle`).
  3. (Optional) Run `make utils` in src/ to build utility programs.
- **Run:**
  - Start the MUD server with `bin/circle` (default port is 4000).
- **Connect:**
  - Use a Telnet client: `telnet localhost 4000`.
- **Logs:**
  - Logs are in `mud/log/`.
- **Troubleshooting:**
  - If you see missing includes (e.g., `strlen`, `strcpy`), add `#include <string.h>` to the relevant C file.
  - See `mud/README.md` and `doc/` for more details.

### 2. Web Interface (server/)
- **Install dependencies:**
  - Run `yarn install` in the server/ directory.
- **Development:**
  - Start the dev server with `yarn dev` (requires MUD server running on port 4000).
  - Access at http://localhost:3000
- **Production Build:**
  - Build with `yarn build`, then start with `yarn start`.
- **Testing:**
  - Run unit tests: `yarn test`
  - Run end-to-end tests: `yarn test:e2e` (requires both servers running)
  - Playwright and Jest are used for testing.
- **Docker:**
  - Use `docker-compose -f docker-compose.dev.yml up` for development.
  - Use `docker-compose -f docker-compose.test.yml up` for testing.
- **Environment:**
  - Copy `.env.example` to `.env.local` and edit as needed.
- **Project structure and more details:**
  - See `server/README.md` for architecture, features, and troubleshooting.

### 3. Combined Launch
- Use `launch.sh` in the project root to start both the MUD server and the web interface together.
- For deployment, see `DOKKU.md` for Dokku-specific instructions and persistent storage setup.

---

Update this file with any new project-specific instructions or preferences as they arise.
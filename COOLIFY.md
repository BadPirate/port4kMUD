# Deploying Port4kMUD to Coolify

[Coolify](https://coolify.io) is the primary supported way to run Port4kMUD.
It builds the repository's `Dockerfile`, gives you a persistent volume for the
game data, and terminates TLS for the web portal. A Dokku setup is still
documented in [DOKKU.md](DOKKU.md) as an alternative.

One container runs both halves of the project: `launch.sh` starts the CircleMUD
server on port 4000 in the background, then runs the Next.js web portal in the
foreground on `$PORT`.

## Prerequisites

- A Coolify instance with a server attached
- This repository reachable from Coolify, either through the GitHub App
  integration or as a public repository

## 1. Create the application

In your project, **+ New** → **Application** → pick the Git source and this
repository, then set:

| Field | Value |
| --- | --- |
| **Build Pack** | `Dockerfile` |
| **Base Directory** | `/` |
| **Dockerfile Location** | `/Dockerfile` |
| **Branch** | `main` |

Port4kMUD mixes C and Node.js in one image, so the Dockerfile build pack is
required - Nixpacks will not produce a working container.

## 2. Ports

The portal listens on `$PORT`, defaulting to `3000`, and binds to all
interfaces so Coolify's proxy can reach it.

- **Ports Exposes**: `3000`. This is the port the proxy forwards your domain
  to, and the one the health check uses.
- **Ports Mappings**: `4000:4000`. Telnet is a raw TCP protocol, so the HTTP
  proxy will not route it; without this mapping the game is reachable only
  through the browser client. Change the left-hand number if port 4000 is
  already taken on the host.

## 3. Persistent storage

Everything the game writes - the world, player files, and the portal's account
database - lives under `mud/lib`. Without volumes, every player and every OLC
edit is lost on redeploy.

In the **Persistent Storage** panel add two volume mounts:

| Name | Destination Path |
| --- | --- |
| `port4kmud-lib` | `/app/mud/lib` |
| `port4kmud-log` | `/app/mud/log` |

No further setup is needed. The image ships the initial game data as
`/app/mud/lib-dist`, and `launch.sh` copies it into `/app/mud/lib` the first
time it finds the volume empty. On later boots it leaves the volume alone.

The portal's account database is a SQLite file at
`mud/lib/etc/portal.sqlite`, inside the same volume, so accounts survive a
redeploy with no extra configuration. `launch.sh` applies any pending
migrations on every boot.

## 4. Environment variables

Set these under **Environment Variables**.

### Required

```
BETTER_AUTH_SECRET=<generate with: openssl rand -base64 32>
```

The app refuses to start in production without it. It signs session cookies
**and** encrypts the MUD passwords the portal replays at the game's own login
prompt, so changing it signs everyone out and makes every saved character
password unreadable - each character then has to be entered again. Back it up
somewhere safe.

### Required for sign-in to work

Sign-in is by emailed magic link. With no `SMTP_HOST` no mail is sent, and
nobody can sign in. The game itself is still playable without an account.

```
SMTP_HOST=smtp.example.com
SMTP_PORT=587
SMTP_USER=port4k@example.com
SMTP_PASS=<your smtp password>
SMTP_FROM=Port4k <port4k@example.com>
```

### Optional

```
SITE_TITLE=Port 4000                 # navbar title; defaults to "telnet <host> 4000"
MAX_CHARACTER_LIMIT=5                # characters per account
MAGIC_LINK_TTL_SECONDS=900           # how long a sign-in link stays usable
BETTER_AUTH_URL=https://mud.example.com   # only if the public URL differs from the request host
AUTH_DATABASE_PATH=/app/mud/lib/etc/portal.sqlite   # only to move the database
PORT=3000                            # must match Ports Exposes if you set it
```

`NODE_ENV=production` is already set in the image.

## 5. Domain and HTTPS

Point a DNS record at your Coolify server, then set **Domains** to
`https://mud.example.com`. Coolify's proxy requests a Let's Encrypt
certificate automatically. Telnet on port 4000 is not proxied and is not
encrypted - that is normal for a MUD.

## 6. Deploy

Press **Deploy**. The first build compiles the MUD with autoconf/make and
builds the Next.js app, so it takes a few minutes; later builds reuse the
cached dependency layers.

Watch the logs for:

```
===== Starting MUD Server =====
MUD server started with PID: ...
===== Applying portal database migrations =====
===== Starting Web Interface =====
> Ready on http://... - env production
```

Then open your domain in a browser, and check `telnet mud.example.com 4000`.

## 7. Updating

Push to the deployment branch. If you enabled **Automatic Deployment**,
Coolify rebuilds on the webhook; otherwise press **Redeploy**.

## Rebuilding from inside the game: `copyover`

An implementor can run `copyover <branch>` in-game. This is not a normal
restart - the MUD pulls `mud/` from that branch of the public GitHub
repository, runs `autoconf`, `./configure` and `make`, and only then execs the
new binary, handing it the live player sockets. Nobody gets disconnected, but
the game freezes for the length of the build.

Two consequences worth knowing:

- The container needs the C toolchain (`gcc`, `make`, `autoconf`) and `git` at
  runtime, which is why the image carries them. They are also the reason the
  image is around a gigabyte rather than a few hundred megabytes.
- A `copyover` build is written into the container's filesystem, not into a
  volume, so it survives until the next redeploy or container restart - at
  which point the image's own binary comes back. Merge the branch and redeploy
  to make a change permanent.
- If the branch's `lib/world` differs from the live world, `copyover` stops and
  asks you to repeat the command with `confirm`, so that OLC edits sitting on
  the volume are not silently overwritten.

The build log is at `/tmp/port4k-copyover-build.log` inside the container.

## Backups

Back up the `port4kmud-lib` volume - it holds the world, every player file,
and `etc/portal.sqlite`. From a shell on the Coolify server:

```bash
docker run --rm -v port4kmud-lib:/data -v "$PWD":/backup busybox \
  tar -czf /backup/port4kmud-$(date +%Y%m%d).tar.gz -C /data .
```

Store `BETTER_AUTH_SECRET` with the backup; without it the saved character
passwords in that database cannot be decrypted.

## Troubleshooting

**The portal loads but the terminal will not connect.** The MUD process is not
running. `launch.sh` starts it in the background and does not supervise it, so
the container stays up either way. Open a terminal on the container and check:

```bash
cat /app/mud/log/syslog
ls -l /app/mud/lib/text/motd    # should exist after the first boot
```

**"Fatal error changing to data directory".** The `lib` volume is mounted but
was not seeded. Confirm `/app/mud/lib-dist` exists in the image and that
`/app/mud/lib` is writable by the container.

**Deploy fails during `yarn install` or `make`.** The build needs outbound
network access for the package registry, and `copyover` needs it for
`github.com`. Check the server's egress rules.

**Players lost after a redeploy.** The `lib` volume was not mounted, or was
mounted at the wrong path. It must be exactly `/app/mud/lib`.

# Multi-stage build. The three build stages exist so that the compilers and
# dependencies each of them needs stay out of the final image; only the last
# stage is shipped.
#
# Every stage shares one base image so that the native modules built in
# `server-deps` (better-sqlite3, and the Prisma client's generated output) stay
# ABI-compatible with the Node that runs them.
ARG BASE_IMAGE=node:22-bookworm-slim


##############################################################################
# Stage 1 - compile the MUD.
##############################################################################
FROM ${BASE_IMAGE} AS mud-build

# The MUD is plain C (CircleMUD, -std=gnu89) and links against libc and
# -lcrypt only - see mud/configure.in and mud/src/Makefile.in. No C++.
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc \
    make \
    libc6-dev \
    libcrypt-dev \
    autoconf \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app/mud
COPY mud/ ./

# bin/ is excluded by .dockerignore (it holds only a .gitignore in a fresh
# checkout), but src/Makefile links straight to ../bin/circle and fails if the
# directory is missing.
RUN mkdir -p bin && \
    autoconf && \
    ./configure && \
    make -C src

# The game data ships as lib-dist, not lib: a deployment mounts a persistent
# volume over /app/mud/lib, so a copy at that path would be shadowed and never
# read. launch.sh restores lib-dist into lib when the volume comes up empty.
#
# The object files go because `copyover` replaces the whole source tree with a
# fresh checkout, whose newer timestamps force a full rebuild anyway.
RUN mv lib lib-dist && rm -f src/*.o


##############################################################################
# Stage 2 - build the web application.
##############################################################################
FROM ${BASE_IMAGE} AS server-build

# node-gyp's toolchain, for the platforms where better-sqlite3 ships no
# matching prebuilt binary.
RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 \
    make \
    g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app/server

# Copied ahead of the rest of the source so that editing application code does
# not invalidate the install layer. prisma.config.ts, prisma/ and
# auth-db-path.ts are in here because `postinstall` runs `prisma generate`,
# which loads prisma.config.ts, which imports ./src/utils/auth-db-path.
COPY server/package.json server/yarn.lock server/prisma.config.ts ./
COPY server/prisma ./prisma
COPY server/src/utils/auth-db-path.ts ./src/utils/auth-db-path.ts
# The cache is cleaned here too. It never ships - only the runtime stage is
# exported - but at ~2.5GB for this dependency set it is a lot of builder disk
# on a CI runner, and it would be carried by a registry build cache.
RUN yarn install --frozen-lockfile && yarn cache clean

COPY server/ ./
RUN yarn build

# node_modules is rebuilt from scratch in server-deps without the dev half, and
# .next/cache is webpack's incremental-build state, which the runtime never
# reads. Removing them here keeps them out of what the runtime stage copies.
RUN rm -rf node_modules .next/cache


##############################################################################
# Stage 3 - resolve the production dependency tree.
##############################################################################
# Separate from stage 2 so the dev dependencies (eslint, jest, prettier, the
# testing-library packages, ...) and yarn's download cache never reach the
# runtime stage. BuildKit runs this concurrently with stage 2.
#
# It is not a perfect filter: yarn v1 keeps a devDependency that satisfies a
# production package's peerDependency, so typescript (peer of @prisma/client,
# prisma and ts-node) and @playwright/test (peer of next) still ship, about
# 41MB between them. Neither is loaded at runtime - `prisma migrate deploy`
# was checked against a tree with typescript removed - but deleting packages
# that are there to satisfy a peer is not worth 3% of the image.
FROM ${BASE_IMAGE} AS server-deps

RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 \
    make \
    g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app/server

COPY server/package.json server/yarn.lock server/prisma.config.ts ./
COPY server/prisma ./prisma
COPY server/src/utils/auth-db-path.ts ./src/utils/auth-db-path.ts

# `yarn cache clean` matters as much as --production here: yarn keeps every
# tarball it downloaded, which for this dependency set is ~2.5GB, and a
# single-stage build ships all of it.
#
# The `prisma` CLI is a production dependency because launch.sh runs
# `prisma migrate deploy` on every boot. That costs roughly 300MB, because the
# CLI eagerly requires @prisma/studio-core and drags in Prisma Studio's React
# UI stack. Deleting those packages afterwards does not work - the CLI fails to
# load at all - so the cost is unavoidable while migrations run at startup.
RUN yarn install --production --frozen-lockfile && yarn cache clean


##############################################################################
# Stage 4 - the image that actually ships.
##############################################################################
FROM ${BASE_IMAGE} AS runtime

# This toolchain is a runtime requirement, not a build one. The in-game
# `copyover <branch>` command (mud/src/copyover_update.c, reached from
# do_copyover in mud/src/act.wizard.c) re-clones mud/ from GitHub and runs
# autoconf, ./configure and make before exec'ing the rebuilt binary onto the
# live sockets. There is no form of the command that skips the rebuild, so
# without these it fails outright.
#
# gcc alone is enough - the MUD is plain C. g++ and python3 are needed only by
# node-gyp during the build, and stay in stages 2 and 3.
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc \
    make \
    libc6-dev \
    libcrypt-dev \
    autoconf \
    git \
    ca-certificates \
    diffutils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=mud-build /app/mud ./mud

# All of server/ rather than an enumerated list of dist/.next/public/prisma:
# the source is about a megabyte, and the Prisma CLI resolves prisma.config.ts
# and its imports at runtime, so a hand-written list is one refactor away from
# breaking migrations on boot.
COPY --from=server-build /app/server ./server
COPY --from=server-deps /app/server/node_modules ./server/node_modules

COPY launch.sh Procfile ./
RUN chmod +x launch.sh

ENV NODE_ENV=production

# The web interface listens on $PORT, which defaults to 3000; a proxy forwards
# the public domain to it. The MUD's telnet port is 4000 and needs a raw TCP
# mapping - an HTTP proxy will not route it.
EXPOSE 3000
EXPOSE 4000

ENTRYPOINT ["./launch.sh"]

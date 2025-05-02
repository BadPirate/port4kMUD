FROM node:20-bullseye-slim

# Install necessary dependencies for building the MUD server
RUN apt-get update && apt-get install -y \
    build-essential \
    libc6-dev \
    make \
    gcc \
    g++ \
    libcrypt-dev \
    autoconf \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the entire project
COPY . .

# Set executable permissions on the launch script
RUN chmod +x launch.sh

# Regenerate the configure script and build the MUD server
RUN cd mud && \
    autoconf && \
    ./configure && \
    cd src && \
    make

# Create a lib-dist directory with essential MUD files
RUN mkdir -p mud/lib-dist && \
    cp -r mud/lib/* mud/lib-dist/ || true

# Install and build the web application
RUN cd server && \
    yarn install && \
    yarn build

# Expose ports for the MUD server (telnet) and web interface (HTTP)
EXPOSE 4000
# Use a placeholder - the actual port will be set by Dokku via PORT env variable
EXPOSE 80

# Set the launch script as the entrypoint
ENTRYPOINT ["./launch.sh"]
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

# Run autoconf to generate configure script from configure.in
# Then run configure and make to build the MUD server
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

# Install and build the web application, expose before 4000 to avoid conflicts
EXPOSE 80

# Expose ports for the MUD server (telnet) and web interface (HTTP)
EXPOSE 4000
# Use a placeholder - the actual port will be set by Dokku via PORT env variable

# Set the launch script as the entrypoint
ENTRYPOINT ["./launch.sh"]
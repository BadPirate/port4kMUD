FROM node:20-bullseye-slim

# Install necessary dependencies for building the MUD server
RUN apt-get update && apt-get install -y \
    build-essential \
    libc6-dev \
    make \
    gcc \
    g++ \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the entire project
COPY . .

# Set executable permissions on the launch script
RUN chmod +x launch.sh

# Build the MUD server in advance
RUN cd mud && \
    ./configure && \
    cd src && \
    make

# Install and build the web application
RUN cd server && \
    yarn install && \
    yarn build

# Dokku default, but this is just a hint, will use PORT env var for www
EXPOSE 3000

# Expose ports for the MUD server (telnet) and web interface (HTTP)
EXPOSE 4000

# Set the launch script as the entrypoint
ENTRYPOINT ["./launch.sh"]
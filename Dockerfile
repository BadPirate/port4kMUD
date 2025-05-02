FROM node:20-bullseye-slim

# Install necessary dependencies for building the MUD server
RUN apt-get update && apt-get install -y \
    build-essential \
    libc6-dev \
    make \
    gcc \
    g++ \
    libcrypt-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the entire project
COPY . .

# Set executable permissions on the launch script
RUN chmod +x launch.sh

# Configure and build the MUD server with explicit crypt library linking
RUN cd mud && \
    ./configure && \
    cd src && \
    # Modify the Makefile to add -lcrypt to LIBS
    sed -i 's/^LIBS = \(.*\)/LIBS = \1 -lcrypt/' Makefile && \
    make

# Install and build the web application
RUN cd server && \
    yarn install && \
    yarn build

# Expose ports for the MUD server (telnet) and web interface (HTTP)
EXPOSE 4000
# Dokku default, but this is just a hint, will use PORT env var for www
EXPOSE 3000

# Set the launch script as the entrypoint
ENTRYPOINT ["./launch.sh"]
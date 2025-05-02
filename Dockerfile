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

# Configure and build the MUD server with explicit linking to crypt
RUN cd mud && \
    ./configure && \
    cd src && \
    # Modify the compile command for ../bin/circle directly
    sed -i 's/\(^\t\$(CC).*\$(OBJFILES)\)/\1 -lcrypt/' Makefile && \
    make

# Install and build the web application
RUN cd server && \
    yarn install && \
    yarn build

# Expose ports for the MUD server (telnet) and web interface (HTTP)
EXPOSE 4000
# Use a placeholder - the actual port will be set by Dokku via PORT env variable
# This is just documentation that the container serves web content
EXPOSE 80

# Set the launch script as the entrypoint
ENTRYPOINT ["./launch.sh"]
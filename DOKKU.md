# Deploying Port4kMUD to Dokku

This guide provides step-by-step instructions for deploying Port4kMUD to a Dokku server, including setting up persistent storage for MUD data and configuring the application properly.

## Prerequisites

- A server with [Dokku installed](https://dokku.com/docs/getting-started/installation/)
- SSH access to your Dokku server
- Git installed on your local machine
- Basic familiarity with Dokku commands

## 1. Create the Dokku Application

SSH into your Dokku server and create a new application:

```bash
# Connect to your Dokku server
ssh dokku@your-dokku-server.com

# Create a new application
dokku apps:create port4kmud
```

## 2. Configure Dockerfile Deployment

Port4kMUD contains both C code (for the MUD server) and Node.js code (for the web interface), so we need to use a Dockerfile for deployment instead of relying on auto-detected buildpacks:

```bash
# Configure the app to use Docker deployment
dokku config:set port4kmud DOKKU_BUILDER_TYPE=dockerfile

# Force the builder to rebuild
dokku builder:set port4kmud selected dockerfile
```

## 3. Set Up Persistent Storage

The MUD server requires persistent storage for player data, world state, logs, and other runtime files. You'll need to create directories on your Dokku host and mount them into the container.

```bash
# Create directories on the Dokku host for persistent data
mkdir -p /var/lib/dokku/data/storage/port4kmud/lib
mkdir -p /var/lib/dokku/data/storage/port4kmud/log

# Set appropriate permissions - use the dokku user and group
# 755 for directories (rwx for owner, r-x for group and others)
sudo chown -R dokku:dokku /var/lib/dokku/data/storage/port4kmud
find /var/lib/dokku/data/storage/port4kmud -type d -exec chmod 755 {} \;
# 644 for files (rw- for owner, r-- for group and others)
find /var/lib/dokku/data/storage/port4kmud -type f -exec chmod 644 {} \;

# Mount these directories into the container
dokku storage:mount port4kmud /var/lib/dokku/data/storage/port4kmud/lib:/app/mud/lib
dokku storage:mount port4kmud /var/lib/dokku/data/storage/port4kmud/log:/app/mud/log
```

### Important MUD Files to Persist

The mounted directories will store important MUD files:

- `/mud/lib/` - This entire directory contains:
  - World data and zone files (editable through OLC)
  - Player files and data in `/lib/etc/`
  - Player objects in `/lib/plrobjs/`
  - Player housing files in `/lib/house/`
  - Text files in `/lib/text/`
  - Board messages in `/lib/etc/board.*`
  - And other essential game data
- `/mud/log/` - MUD logs and system messages

## 4. Configure Environment Variables

Set necessary environment variables for your application:

```bash
# Set Node environment to production
dokku config:set port4kmud NODE_ENV=production

# If using authentication or other services, add those variables here
# dokku config:set port4kmud AUTH_SECRET=your_auth_secret
# dokku config:set port4kmud DATABASE_URL=your_database_url
```

## 5. Configure Domain (Optional)

If you want to use a custom domain:

```bash
# Add your domain
dokku domains:add port4kmud your-mud-domain.com
```

## 6. Enable HTTPS (Optional but Recommended)

To secure your MUD web interface with HTTPS:

```bash
# Install the Let's Encrypt plugin if not already installed
sudo dokku plugin:install https://github.com/dokku/dokku-letsencrypt.git

# Set an email for Let's Encrypt
dokku config:set --no-restart port4kmud DOKKU_LETSENCRYPT_EMAIL=your-email@example.com

# Enable Let's Encrypt
dokku letsencrypt:enable port4kmud

# Auto-renew certificates
dokku letsencrypt:cron-job --add
```

## 7. Deploy the Application

On your local machine, add the Dokku remote to your repository:

```bash
# Add the Dokku remote
git remote add dokku dokku@your-dokku-server.com:port4kmud

# Push to deploy
git push dokku main
```

The deployment will use:
1. The `Dockerfile` to build the environment with both C and Node.js support
2. The `launch.sh` script to:
   - Build and start the MUD server in the background
   - Build and start the Next.js web interface

## 8. Prepare Initial MUD Data (First Deployment Only)

After the first deployment, you need to initialize the MUD data files. This step is critical because the persistent directory might be empty on first launch:

```bash
# SSH into the Dokku app to set up initial MUD files
dokku enter port4kmud

# Once inside the container, copy initial files to the persistent storage
cd /app/mud
cp -rn lib/* /app/mud/lib/  # Copy recursively without overwriting existing files
```

Make sure the initial lib directory is properly set up before players connect to your MUD.

## 9. Monitoring and Maintenance

### View Logs

```bash
# View all logs (both web interface and MUD)
dokku logs port4kmud -t

# View recent logs
dokku logs port4kmud -n 100
```

### Restart the Application

```bash
# Restart if needed
dokku ps:restart port4kmud
```

### Check Application Status

```bash
# Check if application is running
dokku ps:report port4kmud
```

## 10. Updating the Application

To update your MUD after making changes:

```bash
# Simply push to the Dokku remote again
git push dokku main
```

## 11. Backing Up MUD Data

It's crucial to regularly back up your MUD data:

```bash
# On the Dokku host, back up the persistent data directories
tar -czf port4kmud-backup-$(date +%Y%m%d).tar.gz /var/lib/dokku/data/storage/port4kmud
```

Consider setting up a cron job for regular backups.

## Troubleshooting

### Build Failures

If deployment fails with buildpack errors:

```bash
# Check if Docker deployment is properly configured
dokku builder:report port4kmud

# Verify Docker is being used
dokku config:get port4kmud DOKKU_BUILDER_TYPE
```

### MUD Server Not Starting

If the MUD server doesn't start:

```bash
# Check MUD-specific logs
dokku logs port4kmud -t | grep "MUD server"

# SSH into the container to check manually
dokku enter port4kmud
cd /app/mud
cat log/syslog
```

### Connection Issues Between Web Interface and MUD

If the web interface can't connect to the MUD server:

```bash
# Ensure the MUD server is running on port 4000
dokku enter port4kmud
netstat -tuln | grep 4000

# Check for any connection errors in the logs
dokku logs port4kmud -t | grep "MUD connection"
```

### Persistent Storage Issues

If files aren't being saved properly:

```bash
# Check mount points inside the container
dokku enter port4kmud
ls -la /app/mud/lib/etc
ls -la /app/mud/log

# Verify permissions on the host
ls -la /var/lib/dokku/data/storage/port4kmud
```

## Advanced Configuration

### Resource Allocation

If you need to allocate specific resources to your MUD:

```bash
# Set memory limit (e.g., 1GB)
dokku resource:limit --memory 1G port4kmud

# Set CPU limit (e.g., 2 CPUs)
dokku resource:limit --cpu 2 port4kmud
```

### Enabling Direct Telnet Access

By default, players can access the MUD through the web interface. However, many seasoned MUD players prefer connecting directly via a telnet client. To enable direct telnet access:

```bash
# Expose the MUD's telnet port (4000) to the outside world
dokku proxy:ports-add port4kmud tcp:4000:4000
```

After running this command, players can connect directly to your MUD using any telnet client:

```
telnet your-dokku-server.com 4000
```

For better security, you can also choose to use a different external port:

```bash
# Map external port 14000 to container's port 4000
dokku proxy:ports-add port4kmud tcp:14000:4000
```

Then players would connect using:

```
telnet your-dokku-server.com 14000
```

Note: The CircleMUD is hardcoded to use port 4000 internally, but Dokku's port mapping allows you to expose it on any external port.

### Scaled Deployment (Not Recommended for MUD)

MUDs generally don't scale horizontally well due to their stateful nature. It's recommended to keep a single instance running to avoid data inconsistencies.
/** @type {import('next').NextConfig} */
const nextConfig = {
  reactStrictMode: true,
  // server.ts is a custom server; keeping this here rather than in next()'s
  // `conf` means `next build` and the running server agree on it.
  skipTrailingSlashRedirect: true,
}

export default nextConfig

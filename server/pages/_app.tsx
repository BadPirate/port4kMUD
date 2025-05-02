/* eslint-disable react/jsx-props-no-spreading */
import { SessionProvider } from 'next-auth/react'
import '../styles/bootstrap.min.css'
import '../styles/global.css'
import '../styles/xterm.css'

import type { AppProps } from 'next/app'

const App = ({ Component, pageProps }: AppProps) => (
  <SessionProvider>
    <Component {...pageProps} />
  </SessionProvider>
)

export default App

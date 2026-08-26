/* eslint-disable react/jsx-props-no-spreading */
import '../styles/bootstrap.min.css'
import '../styles/global.css'
import '../styles/xterm.css'

import type { AppProps } from 'next/app'

const App = ({ Component, pageProps }: AppProps) => <Component {...pageProps} />

export default App

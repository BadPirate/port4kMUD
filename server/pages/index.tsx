import Head from 'next/head'
import MudClient from '@/src/components/MudClient'

const Index = () => (
  <div style={{ height: '100vh', width: '100vw', padding: '2em' }}>
    <Head>
      <title>Welcome to Port 4k!</title>
    </Head>
    <MudClient />
  </div>
)

export default Index

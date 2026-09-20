import Head from 'next/head'
import type { GetServerSideProps } from 'next'
import MudClient from '@/src/components/MudClient'
import config from '@/src/utils/config'

interface IndexProps {
  title: string
}

const Index = ({ title }: IndexProps) => (
  <>
    <Head>
      <title>{title}</title>
    </Head>
    <MudClient title={title} />
  </>
)

/**
 * The title is read per request rather than at build time: a deployment sets
 * SITE_TITLE in the container's environment, which a build in an image would
 * never have seen. Without one, the page tells players how to reach the same
 * game with a telnet client on whatever host they asked for.
 */
export const getServerSideProps: GetServerSideProps<IndexProps> = async ({ req }) => {
  const host = (req.headers.host || 'localhost').split(':')[0]
  return { props: { title: config.SITE_TITLE || `telnet ${host} 4000` } }
}

export default Index

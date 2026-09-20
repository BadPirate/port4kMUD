import Head from 'next/head'
import type { GetServerSideProps } from 'next'
import MudClient from '@/src/components/MudClient'
import config from '@/src/utils/config'
import { readPortalUserId } from '@/src/utils/portal-request'

interface IndexProps {
  title: string
  signedInAtLoad: boolean
}

const Index = ({ title, signedInAtLoad }: IndexProps) => (
  <>
    <Head>
      <title>{title}</title>
    </Head>
    <MudClient title={title} signedInAtLoad={signedInAtLoad} />
  </>
)

/**
 * The title is read per request rather than at build time: a deployment sets
 * SITE_TITLE in the container's environment, which a build in an image would
 * never have seen. Without one, the page tells players how to reach the same
 * game with a telnet client on whatever host they asked for.
 *
 * Whether anybody is signed in is read here for the same reason it is read at
 * all: the browser only finds out a round trip after the page is on screen,
 * and the page would rather not offer to sign in somebody who already is.
 */
export const getServerSideProps: GetServerSideProps<IndexProps> = async ({ req }) => {
  const host = (req.headers.host || 'localhost').split(':')[0]
  return {
    props: {
      title: config.SITE_TITLE || `telnet ${host} 4000`,
      signedInAtLoad: Boolean(await readPortalUserId(req)),
    },
  }
}

export default Index

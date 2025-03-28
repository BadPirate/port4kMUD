import Head from 'next/head'
import RootNav from './RootNav'

const Page = ({ title, children }: {title: string, children: React.ReactNode}) => (
  <RootNav>
    <Head>
      <title>{title}</title>
    </Head>
    {children}
  </RootNav>
)
export default Page

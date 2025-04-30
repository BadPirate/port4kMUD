import Page from '../src/components/Page'
import WelcomeCard from '../src/cards/WelcomeCard'
import config from '../src/utils/config'

const Index = () => (
  <Page title={`Welcome to ${config.NEXT_PUBLIC_APP_NAME}`}>
    <WelcomeCard />
  </Page>
)

export default Index

const { chromium } = require('@playwright/test');
(async () => {
  const browser = await chromium.launch();
  const page = await browser.newPage();
  await page.goto('http://localhost:3000/api/auth/signin/email?callbackUrl=/');
  await page.fill('input[type="email"]', 'test@example.com');
  await page.click('button[type="submit"]');
  await page.waitForTimeout(3000);
  console.log(await page.content());
  await browser.close();
})();

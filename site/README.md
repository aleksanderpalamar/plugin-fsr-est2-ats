# NeuralFX storefront

This folder contains the static storefront for GitHub Pages. It uses HTML, CSS, and JavaScript for the site, plus an SVG favicon. The Portuguese page is `index.html`; the English page is `en/index.html`.

## Preview locally

From the repository root, run `python3 -m http.server 8000 --directory site` and open `http://localhost:8000/`. Run `node site/tests/checkout.test.js` to check the purchase-link behavior.

## Connect checkout and download

The purchase buttons are enabled at the owner's request. The live Stripe Payment Link is set in `checkout-config.js`. Confirm that it sells the plugin for a one-time USD 5 payment.

1. Build the ZIP with `./scripts/package.sh` if needed. Upload `dist/FSR-ets2-ats-1.0.0.zip` to the Google Drive folder used for delivery.
2. Set the folder's **General access → Anyone with the link → Viewer**. The buyer may need to sign in to a Google account, but signing in alone does not grant access to a restricted folder. Anyone who receives a working folder URL can share it, and future files added to the folder inherit its permissions. Google may show the owner's name and email address.
3. In the existing [Stripe Payment Link](https://dashboard.stripe.com/payment-links), check **After the payment** and its redirect to the Drive folder. [Stripe's post-payment guide](https://docs.stripe.com/payment-links/post-payment) describes this setting.
4. Use payment methods with immediate confirmation for this redirect-only flow. Methods with delayed confirmation require a webhook to verify payment before delivery. Complete a purchase with a different Google account and confirm that Stripe opens the correct ZIP in Drive. An unauthenticated check of the current folder URL was redirected to Google sign-in; access from another Google account has not been independently verified. Redirects do not send an email with the file, and buyers who close the browser before the redirect may need support to get the link.
5. If delivery fails, set `deliveryReady` to `false` in `checkout-config.js` to disable both purchase links. Test-mode, non-Stripe, and insecure payment URLs stay inactive.

The site never contains a Stripe secret key or a public link to the ZIP. It does not attempt to confirm payments in the browser. The Stripe redirect supplies the Drive folder link to buyers, but it does not restrict future access to that link.

GitHub Actions publishes the site; it does not receive Stripe payment events or grant access to Drive files. Per-buyer Drive access would require a payment-verifying service and the Google Drive API.

## Publish with GitHub Pages

In repository **Settings → Pages**, choose **GitHub Actions** as the build and deployment source. The workflow in `.github/workflows/pages.yml` publishes this folder when changes to `site/` land on `main`; it can also run manually. GitHub Pages cannot choose an arbitrary `site/` folder as a branch publishing source.

The project source is public under the MIT license. The paid offer is the ready-to-install package, and the payment supports continued development. Keep that distinction clear in any sales copy.

Before starting paid delivery, review the license notice required by `LICENSE` and the redistribution terms for the bundled third-party LUT. The current package does not carry separate license or third-party notice files.

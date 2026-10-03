const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const test = require("node:test");
const vm = require("node:vm");

const script = fs.readFileSync(path.join(__dirname, "../main.js"), "utf8");
const configScript = fs.readFileSync(path.join(__dirname, "../checkout-config.js"), "utf8");

function renderCheckout(config) {
  const links = ["Comprar por US$ 5", "Comprar com Stripe"].map((readyLabel) => ({
    dataset: { readyLabel },
    textContent: "Compra em preparação",
    disabled: true,
    removeAttribute(name) {
      if (name === "aria-disabled") this.disabled = false;
    }
  }));
  const status = { dataset: { readyText: "Checkout seguro" }, textContent: "Em preparação" };
  const document = {
    querySelectorAll(selector) {
      return selector === "[data-checkout]" ? links : [status];
    }
  };

  vm.runInNewContext(script, { URL, window: { NEURALFX_CHECKOUT: config }, document });
  return { links, status };
}

test("keeps checkout unavailable without a payment link", () => {
  const result = renderCheckout({ paymentLink: "", deliveryReady: false });
  assert.equal(result.links[0].href, undefined);
  assert.equal(result.links[0].disabled, true);
  assert.equal(result.status.textContent, "Em preparação");
});

test("keeps checkout unavailable until digital delivery is ready", () => {
  const result = renderCheckout({ paymentLink: "https://buy.stripe.com/valid123", deliveryReady: false });
  assert.equal(result.links[0].href, undefined);
});

test("enables both purchase links for a live Stripe Payment Link", () => {
  const result = renderCheckout({ paymentLink: "https://buy.stripe.com/valid123", deliveryReady: true });
  assert.deepEqual(result.links.map((link) => link.href), ["https://buy.stripe.com/valid123", "https://buy.stripe.com/valid123"]);
  assert.deepEqual(result.links.map((link) => link.disabled), [false, false]);
  assert.equal(result.links[0].textContent, "Comprar por US$ 5");
  assert.equal(result.status.textContent, "Checkout seguro");
});

test("connects the current checkout configuration to both purchase links", () => {
  const window = {};
  vm.runInNewContext(configScript, { window });
  const result = renderCheckout(window.NEURALFX_CHECKOUT);
  const paymentLink = "https://buy.stripe.com/dRm28l4OE7nT9SIcaGbAs01";

  assert.deepEqual(result.links.map((link) => link.href), [paymentLink, paymentLink]);
  assert.deepEqual(result.links.map((link) => link.disabled), [false, false]);
});

test("rejects external, insecure, and test-mode links", () => {
  const links = ["https://example.com/pay", "http://buy.stripe.com/live123", "https://buy.stripe.com/test_123"];
  links.forEach((paymentLink) => {
    const result = renderCheckout({ paymentLink, deliveryReady: true });
    assert.equal(result.links[0].href, undefined);
  });
});

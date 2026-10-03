function livePaymentLink(config) {
  if (!config || config.deliveryReady !== true) return null;
  if (typeof config.paymentLink !== "string") return null;

  let url;
  try {
    url = new URL(config.paymentLink);
  } catch {
    return null;
  }

  if (url.protocol !== "https:" || url.hostname !== "buy.stripe.com") return null;
  if (url.username || url.password || url.pathname.length < 2) return null;
  if (url.pathname.startsWith("/test_")) return null;
  return url.href;
}

function enableCheckout(paymentLink) {
  document.querySelectorAll("[data-checkout]").forEach((link) => {
    link.href = paymentLink;
    link.removeAttribute("aria-disabled");
    link.textContent = link.dataset.readyLabel;
  });

  document.querySelectorAll("[data-checkout-status]").forEach((status) => {
    status.textContent = status.dataset.readyText;
  });
}

const paymentLink = livePaymentLink(window.NEURALFX_CHECKOUT);
if (paymentLink) enableCheckout(paymentLink);

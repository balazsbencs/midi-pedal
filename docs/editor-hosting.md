# Hosting the browser editor

The editor is a static site. It does not need an API server, database, login,
rewrite rule, service worker, or analytics endpoint. Build it with:

```bash
pnpm install --frozen-lockfile
pnpm --filter @midi-pedal/editor typecheck
pnpm --filter @midi-pedal/editor test
pnpm --filter @midi-pedal/editor build
```

Publish the contents of `editor/dist/` exactly as generated. Vite uses a
relative asset base, so the site can be mounted at `/`, `/midi-pedal/`, or
another static subpath without changing JavaScript URLs.

## Secure context and browser support

WebSerial is supported here in desktop Chromium-based browsers: current
Chrome, Edge, and Brave. The deployed origin must be HTTPS; `http://localhost`
is the development exception. The browser asks for the serial port only after
the user presses **Connect pedal**. A denied prompt can be retried from the
same button; no permission is requested during page load.

Safari, Firefox, mobile browsers, and insecure remote HTTP origins are outside
the v1 support boundary. The editor remains useful for JSON editing and
export when no serial API is present, but it cannot sync to a pedal.

## Cache headers

Serve `index.html` with `Cache-Control: no-cache` so a new deployment is
discoverable. Hashed files under `assets/` can use
`Cache-Control: public, max-age=31536000, immutable`. Set JavaScript and CSS
MIME types normally; no fallback rewrite is needed for this single-page app.

## Examples

Cloudflare Pages and GitHub Pages can use `editor/dist` as the output directory
with `pnpm --filter @midi-pedal/editor build` as the build command. For nginx:

```nginx
location /midi-pedal/ {
    alias /srv/midi-pedal/editor/dist/;
    add_header Cache-Control "no-cache" always;
}
location /midi-pedal/assets/ {
    alias /srv/midi-pedal/editor/dist/assets/;
    add_header Cache-Control "public, max-age=31536000, immutable" always;
}
```

Use the same HTTPS hostname for the page and the browser session. The pedal
itself is connected directly over USB; LAN access to the static site does not
create a network path to the device.


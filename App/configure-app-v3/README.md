# Flow Art Forge — Configurator UI

Flow Art Forge is a Vite + React 19 single-page application for composing LED patterns and device modes before flashing firmware. The current milestone establishes the project scaffold, global theming, routing, localization, and the first interactive settings and serial-log experiences.

## ✨ Highlights

- **Modern toolchain** — React 19, TypeScript, Vite, Tailwind CSS v4, strict ESLint/Prettier, Vitest, Testing Library.
- **Hash-based routing** with a themed navigation shell and localized page titles.
- **Global theming** — Automatic system detection with persistable user overrides powered by zustand.
- **Settings page** exposing a stateless `ThemePreferenceSelector` that emits `(state, action)` pairs per component guidelines.
- **Serial console UI** (`SerialLogPanel`) featuring timestamped logs, autoscroll toggle, clear button, and outbound payload submission.
- **Localization** via i18next + react-i18next with browser language detection and reusable translations.
- **Comprehensive tests** covering layout, theme selector, and serial log behaviors.

## 🚀 Getting started

```bash
npm install
npm run dev    # start Vite dev server at http://localhost:5173
```

The app uses a `HashRouter`, so deep links continue to work on static hosting platforms such as Vercel.

### Recommended scripts

| Script                                    | Description                                      |
| ----------------------------------------- | ------------------------------------------------ |
| `npm run dev`                             | Start the development server with hot reloading. |
| `npm run build`                           | Type-check and create a production build.        |
| `npm run preview`                         | Preview the production build locally.            |
| `npm run lint` / `npm run lint:fix`       | Run ESLint in strict mode (with autofix).        |
| `npm run format` / `npm run format:check` | Apply or verify Prettier formatting.             |
| `npm run test` / `npm run test:watch`     | Execute Vitest suites once or in watch mode.     |

## 🧱 Project structure

```
src/
├── app/
│   ├── layout/           # Application chrome & navigation
│   ├── providers/        # Theme store, ThemeProvider, i18n bootstrap
│   └── router/           # HashRouter configuration & route constants
├── components/
│   ├── serial-log/       # SerialLogPanel component + tests
│   └── settings/         # ThemePreferenceSelector component + tests
├── pages/                # Routed page shells (RGB, Bulb, Mode, Settings, Serial Log, Home)
├── locales/              # i18next translation resources
└── test-utils/           # Shared Testing Library render helper
```

## 🎨 Theming

- The zustand-backed `ThemeProvider` tracks `system`, `light`, or `dark` preferences and applies a `data-theme` attribute to `<html>`.
- Tailwind CSS v4 powers layout and typography while custom CSS variables (`--surface`, `--accent`, etc.) drive light/dark palettes.
- `ThemePreferenceSelector` is a stateless, fully localized radio group that emits both the next state and the triggering action.

## 🌐 Localization

i18next + `react-i18next` provide runtime translations with browser language detection (defaulting to English). Add new strings to `src/locales/en/common.json` and reuse the existing `useTranslation` hooks within components/pages.

## 🔌 Serial console panel

`SerialLogPanel` showcases the stateless component contract:

- Accepts a `value` object (`entries`, `autoscroll`, `pendingPayload`).
- Emits `(newState, action)` callbacks for payload edits, submissions, clears, and autoscroll toggles.
- Auto-scrolls when `autoscroll` is enabled and new entries arrive (with graceful fallback in test environments).

## ✅ Testing & linting

- Vitest + Testing Library verify layout navigation, theme preference handling, and serial log interactions.
- ESLint runs with TypeScript-aware, import-order, accessibility, and React Refresh constraints.
- Prettier enforces formatting; relevant ignores are defined in `.prettierignore`.

## 📍 Roadmap

- Flesh out the RGB pattern, bulb pattern, and mode composition workflows (left as TODO until requirements are provided).
- Persist serial logging to storage or wire to actual device APIs when available.
- Add additional locales as translations become available.

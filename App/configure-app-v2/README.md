# BulbChips — Single Page React App

A lightweight SPA built with Vite + React + TypeScript. Includes ESLint, Prettier, Husky + lint-staged, and Vitest + React Testing Library.

## Tech Stack

- Vite (React + TypeScript)
- TypeScript (strict)
- ESLint (typescript-eslint, react, react-hooks, import, config-prettier)
- Prettier
- Husky + lint-staged (pre-commit)
- Vitest + React Testing Library + jsdom

## Prerequisites

- Node.js 18 or newer
- npm (or pnpm/yarn — scripts use npm examples)

## Getting Started

1. Install dependencies
   - npm install
2. Start the dev server
   - npm run dev
3. Build for production
   - npm run build
4. Preview local production build
   - npm run preview

## Scripts

- dev: starts Vite dev server
- build: builds the app for production
- preview: preview the production build
- typecheck: runs TypeScript in noEmit mode
- lint: runs ESLint
- lint:fix: ESLint with auto-fix
- format: formats files with Prettier
- format:check: checks formatting with Prettier
- test: runs unit tests once
- test:watch: runs unit tests in watch mode
- test:coverage: runs tests with coverage
- prepare: initializes Husky

## Path Aliases

Use `@/` for imports from `src/`. Configure in `tsconfig.json` and `vite.config.ts`.

```ts
import { MyComponent } from '@/components/MyComponent';
```

## Project Structure

- src/
  - main.tsx — app bootstrap
  - App.tsx — root component
  - components/ — shared UI components
  - assets/ — static assets
- index.html — app HTML entry
- vite.config.ts — Vite config (React plugin, alias, Vitest)
- tsconfig.json — strict TypeScript config
- .eslintrc.cjs — ESLint config
- .prettierrc, .prettierignore — Prettier config
- .husky/ — Git hooks (created by Husky)

## ChromaSynth RGB Waveform Editor

The `ChromaSynthEditor` component (`src/components/ChromaSynthEditor.tsx`) renders a three-lane RGB waveform editor with:

- A combined preview strip that animates over time with play/pause controls.
- Separate SVG waveform visualizers and section managers for the red, green, and blue channels.
- Stateless data flow that relies on `ChromaSynthState` props and emits the same shape through the `onChange` callback.

Shapes and helpers live in `src/types/chromaSynth.ts`:

```ts
import type { ChromaSynthState } from '@/types/chromaSynth';

const [value, setValue] = useState<ChromaSynthState>({
   red: { sections: [] },
   green: { sections: [] },
   blue: { sections: [] },
});

<ChromaSynthEditor value={value} onChange={setValue} />;
```

Optionally forward export requests via the `onExport(format, frames)` prop to integrate with rendering or capture pipelines.

## Pre-commit hooks

Husky runs `lint-staged` to lint/format staged files.

If hooks aren't created yet, run:

```bash
npm run prepare
```

This initializes `.husky/`. Then ensure `.husky/pre-commit` runs:

```sh
npx lint-staged
```

## Notes

- React 18, Vite 5, TypeScript 5.
- ESLint 9 + `eslint-config-prettier` to avoid conflicts.
- Tests run in a jsdom environment with `@testing-library/jest-dom`.

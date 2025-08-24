import type { Config } from 'tailwindcss';

export default {
  content: ['./index.html', './src/**/*.{ts,tsx}'],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        bg: {
          DEFAULT: 'rgb(var(--bg) / <alpha-value>)',
          card: 'rgb(var(--bg-card) / <alpha-value>)',
        },
        fg: {
          DEFAULT: 'rgb(var(--fg) / <alpha-value>)',
          muted: 'rgb(var(--fg-muted) / <alpha-value>)',
          ring: 'rgb(var(--fg-ring) / <alpha-value>)',
        },
      },
      borderRadius: {
        xl: '0.75rem',
      },
    },
  },
  plugins: [],
} satisfies Config;

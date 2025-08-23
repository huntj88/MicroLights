import type { Config } from 'tailwindcss';

export default {
  content: ['./index.html', './src/**/*.{ts,tsx}'],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        bg: {
          DEFAULT: '#0f172a',
          card: '#111827',
        },
        fg: {
          DEFAULT: '#e2e8f0',
          muted: '#94a3b8',
          ring: '#60a5fa',
        },
      },
      borderRadius: {
        xl: '0.75rem',
      },
    },
  },
  plugins: [],
} satisfies Config;

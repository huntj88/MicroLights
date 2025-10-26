export const ROUTES = {
  home: '/',
  rgbPattern: 'patterns/rgb',
  bulbPattern: 'patterns/bulb',
  mode: 'modes',
  settings: 'settings',
  serialLog: 'serial-log',
} as const;

type RouteKey = keyof typeof ROUTES;

export const routeOrder: RouteKey[] = ['home', 'rgbPattern', 'bulbPattern', 'mode', 'serialLog', 'settings'];

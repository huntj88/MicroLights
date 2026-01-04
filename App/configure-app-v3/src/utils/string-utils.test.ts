import { describe, expect, it } from 'vitest';

import { humanizeCamelCase } from './string-utils';

describe('humanizeCamelCase', () => {
  it('formats camelCase keys correctly', () => {
    expect(humanizeCamelCase('modeCount')).toBe('Mode Count');
    expect(humanizeCamelCase('minutesUntilAutoOff')).toBe('Minutes Until Auto Off');
  });

  it('formats keys with acronyms correctly', () => {
    expect(humanizeCamelCase('enableChargerSerial')).toBe('Enable Charger Serial');
    expect(humanizeCamelCase('enableUSBSerial')).toBe('Enable USB Serial');
    expect(humanizeCamelCase('useUARTInterface')).toBe('Use UART Interface');
  });

  it('formats simple keys correctly', () => {
    expect(humanizeCamelCase('enabled')).toBe('Enabled');
    expect(humanizeCamelCase('id')).toBe('Id');
  });
});

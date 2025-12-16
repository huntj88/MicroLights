import toast from 'react-hot-toast';
import { MemoryRouter } from 'react-router-dom';
import { describe, expect, it, vi } from 'vitest';

import { renderWithProviders, screen } from '@/test-utils/render-with-providers';

import { AppLayout } from './AppLayout';
import { serialManager } from '../providers/serial-manager';

vi.mock('react-hot-toast');

describe('AppLayout', () => {
  it('renders navigation links with localized labels', () => {
    renderWithProviders(
      <MemoryRouter initialEntries={['/']}>
        <AppLayout />
      </MemoryRouter>,
    );

    expect(screen.getByRole('link', { name: 'nav.home' })).toBeInTheDocument();
    expect(screen.getByRole('link', { name: 'nav.settings' })).toBeInTheDocument();
  });

  it('shows error toast when serial data contains "error"', () => {
    const onSpy = vi.spyOn(serialManager, 'on');

    renderWithProviders(
      <MemoryRouter initialEntries={['/']}>
        <AppLayout />
      </MemoryRouter>,
    );

    const dataListenerCall = onSpy.mock.calls.find(args => args[0] === 'data');
    expect(dataListenerCall).toBeDefined();

    const listener = dataListenerCall?.[1] as (line: string) => void;

    const errorMsg = JSON.stringify({ error: 'Something went wrong error code 123' });
    listener(errorMsg);
    expect(toast.error).toHaveBeenCalledWith(errorMsg);

    listener('Just a normal message');
    expect(toast.error).not.toHaveBeenCalledWith('Just a normal message');
  });
});

/// <reference types="w3c-web-usb" />
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';

import { serialManager } from './serial-manager';

// Mock USBDevice
class MockUSBDevice {
  opened = false;
  open = vi.fn().mockImplementation(async () => {
    this.opened = true;
  });
  close = vi.fn().mockImplementation(async () => {
    this.opened = false;
  });
  selectConfiguration = vi.fn().mockResolvedValue(undefined);
  claimInterface = vi.fn().mockResolvedValue(undefined);
  releaseInterface = vi.fn().mockResolvedValue(undefined);
  transferOut = vi.fn().mockResolvedValue({ status: 'ok' });
  transferIn = vi.fn().mockResolvedValue({
    data: new DataView(new ArrayBuffer(0)),
    status: 'ok',
  });
}

describe('WebUSBManager', () => {
  let mockUsb: {
    requestDevice: ReturnType<typeof vi.fn>;
    addEventListener: ReturnType<typeof vi.fn>;
    removeEventListener: ReturnType<typeof vi.fn>;
  };
  let mockDevice: MockUSBDevice;

  beforeEach(() => {
    mockDevice = new MockUSBDevice();
    mockUsb = {
      requestDevice: vi.fn().mockResolvedValue(mockDevice),
      addEventListener: vi.fn(),
      removeEventListener: vi.fn(),
    };

    // Mock navigator.usb
    Object.defineProperty(navigator, 'usb', {
      value: mockUsb,
      configurable: true,
    });
  });

  afterEach(async () => {
    await serialManager.disconnect();
    vi.clearAllMocks();
  });

  it('reports support correctly', () => {
    expect(serialManager.isSupported()).toBe(true);

    Object.defineProperty(navigator, 'usb', {
      value: undefined,
      configurable: true,
    });
    expect(serialManager.isSupported()).toBe(false);
  });

  it('connects successfully', async () => {
    const statusSpy = vi.fn();
    serialManager.on('connection-status', statusSpy);

    await serialManager.connect();

    expect(mockUsb.requestDevice).toHaveBeenCalledWith({
      filters: [{ vendorId: 0xcafe }],
    });
    expect(mockDevice.open).toHaveBeenCalled();
    expect(mockDevice.selectConfiguration).toHaveBeenCalledWith(1);
    expect(mockDevice.claimInterface).toHaveBeenCalledWith(0);
    expect(statusSpy).toHaveBeenCalledWith('connecting', undefined);
    expect(statusSpy).toHaveBeenCalledWith('connected', undefined);
    expect(serialManager.getStatus()).toBe('connected');
  });

  it('handles connection failure', async () => {
    const error = new Error('User cancelled');
    mockUsb.requestDevice.mockRejectedValue(error);
    const statusSpy = vi.fn();
    serialManager.on('connection-status', statusSpy);

    await expect(serialManager.connect()).rejects.toThrow('User cancelled');

    expect(statusSpy).toHaveBeenCalledWith('connecting', undefined);
    expect(statusSpy).toHaveBeenCalledWith('disconnected', error);
    expect(serialManager.getStatus()).toBe('disconnected');
  });

  it('sends data when connected', async () => {
    await serialManager.connect();

    await serialManager.send('test');

    expect(mockDevice.transferOut).toHaveBeenCalled();
  });

  it('throws when sending while disconnected', async () => {
    await expect(serialManager.send('test')).rejects.toThrow('USB device not connected');
  });

  it('disconnects and cleans up', async () => {
    await serialManager.connect();

    await serialManager.disconnect();

    expect(mockDevice.releaseInterface).toHaveBeenCalledWith(0);
    expect(mockDevice.close).toHaveBeenCalled();
    expect(serialManager.getStatus()).toBe('disconnected');
  });

  it('aborts connection if disconnected while opening', async () => {
    let resolveOpen: ((value: void | PromiseLike<void>) => void) | undefined;
    mockDevice.open.mockImplementation(
      () =>
        new Promise<void>(res => {
          resolveOpen = res;
        }),
    );

    const connectPromise = serialManager.connect();

    // Allow requestDevice to resolve and enter openDevice
    await new Promise(resolve => setTimeout(resolve, 0));

    expect(serialManager.getStatus()).toBe('connecting');

    // Call disconnect — runs synchronously since device isn't assigned yet
    const disconnectPromise = serialManager.disconnect();

    // Finish open
    if (resolveOpen) resolveOpen();

    await expect(connectPromise).rejects.toThrow('Connection aborted');
    await disconnectPromise;

    expect(mockDevice.close).toHaveBeenCalled();
    expect(serialManager.getStatus()).toBe('disconnected');
  });

  it('handles concurrent disconnect calls safely', async () => {
    await serialManager.connect();

    // Mock close to take some time so we can overlap calls
    mockDevice.close.mockImplementation(() => new Promise(res => setTimeout(res, 10)));

    const p1 = serialManager.disconnect();
    const p2 = serialManager.disconnect();

    await Promise.all([p1, p2]);

    expect(mockDevice.close).toHaveBeenCalledTimes(1);
    expect(serialManager.getStatus()).toBe('disconnected');
  });
});

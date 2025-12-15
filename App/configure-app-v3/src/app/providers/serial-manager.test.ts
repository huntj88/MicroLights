/// <reference types="w3c-web-serial" />
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';

import { serialManager } from './serial-manager';

// Mock classes for Stream API
class MockWritableStreamDefaultWriter {
  write = vi.fn().mockResolvedValue(undefined);
  close = vi.fn().mockResolvedValue(undefined);
  releaseLock = vi.fn();
}

class MockReadableStreamDefaultReader {
  read = vi.fn().mockResolvedValue({ value: undefined, done: true });
  cancel = vi.fn().mockResolvedValue(undefined);
  releaseLock = vi.fn();
}

class MockSerialPort {
  readable: ReadableStream | null = null;
  writable: WritableStream | null = null;
  open = vi.fn().mockResolvedValue(undefined);
  close = vi.fn().mockResolvedValue(undefined);

  constructor() {
    // We need to simulate the streams being available after open
    // For simplicity in tests, we can assign them directly or via mocks
    this.readable = {
      getReader: vi.fn().mockReturnValue(new MockReadableStreamDefaultReader()),
      pipeThrough: vi.fn().mockReturnThis(),
    } as unknown as ReadableStream;

    this.writable = {
      getWriter: vi.fn().mockReturnValue(new MockWritableStreamDefaultWriter()),
    } as unknown as WritableStream;
  }
}

describe('WebSerialManager', () => {
  let mockSerial: {
    requestPort: ReturnType<typeof vi.fn>;
    addEventListener: ReturnType<typeof vi.fn>;
    removeEventListener: ReturnType<typeof vi.fn>;
  };
  let mockPort: MockSerialPort;

  beforeEach(() => {
    // Reset singleton state if possible or just rely on isolation
    // Since serialManager is a singleton, we might need to be careful.
    // Ideally, we would export the class to test it in isolation,
    // but for now we will try to reset it via disconnect if needed.

    mockPort = new MockSerialPort();
    mockSerial = {
      requestPort: vi.fn().mockResolvedValue(mockPort),
      addEventListener: vi.fn(),
      removeEventListener: vi.fn(),
    };

    // Mock navigator.serial
    Object.defineProperty(navigator, 'serial', {
      value: mockSerial,
      configurable: true,
    });

    // Mock TextEncoder/Decoder if not present in jsdom (usually they are)
    if (!(globalThis as unknown as { TextEncoder: unknown }).TextEncoder) {
      vi.stubGlobal(
        'TextEncoder',
        class {
          encode = vi.fn().mockReturnValue(new Uint8Array());
        },
      );
    }

    // Mock TransformStream
    if (!(globalThis as unknown as { TransformStream: unknown }).TransformStream) {
      vi.stubGlobal(
        'TransformStream',
        class {
          readable = {};
          writable = {};
        },
      );
    }
  });

  afterEach(async () => {
    await serialManager.disconnect();
    vi.clearAllMocks();
  });

  it('reports support correctly', () => {
    expect(serialManager.isSupported()).toBe(true);

    Object.defineProperty(navigator, 'serial', {
      value: undefined,
      configurable: true,
    });
    expect(serialManager.isSupported()).toBe(false);
  });

  it('connects successfully', async () => {
    const statusSpy = vi.fn();
    serialManager.on('connection-status', statusSpy);

    await serialManager.connect();

    expect(mockSerial.requestPort).toHaveBeenCalled();
    expect(mockPort.open).toHaveBeenCalledWith({ baudRate: 115200 });
    expect(statusSpy).toHaveBeenCalledWith('connecting', undefined);
    expect(statusSpy).toHaveBeenCalledWith('connected', undefined);
    expect(serialManager.getStatus()).toBe('connected');
  });

  it('handles connection failure', async () => {
    const error = new Error('User cancelled');
    mockSerial.requestPort.mockRejectedValue(error);
    const statusSpy = vi.fn();
    serialManager.on('connection-status', statusSpy);

    await expect(serialManager.connect()).rejects.toThrow('User cancelled');

    expect(statusSpy).toHaveBeenCalledWith('connecting', undefined);
    expect(statusSpy).toHaveBeenCalledWith('disconnected', error);
    expect(serialManager.getStatus()).toBe('disconnected');
  });

  it('sends data when connected', async () => {
    // Setup the writer mock before connecting
    const mockWriter = new MockWritableStreamDefaultWriter();
    (
      mockPort.writable as unknown as { getWriter: ReturnType<typeof vi.fn> }
    ).getWriter.mockReturnValue(mockWriter);

    await serialManager.connect();

    await serialManager.send('test');

    expect(mockWriter.write).toHaveBeenCalled();
  });

  it('throws when sending while disconnected', async () => {
    await expect(serialManager.send('test')).rejects.toThrow('Serial port not connected');
  });

  it('disconnects and cleans up', async () => {
    await serialManager.connect();

    // Let's just verify the port close is called
    await serialManager.disconnect();

    expect(mockPort.close).toHaveBeenCalled();
    expect(serialManager.getStatus()).toBe('disconnected');
  });

  it('aborts connection if disconnected while opening', async () => {
    let resolveOpen: ((value: void | PromiseLike<void>) => void) | undefined;
    mockPort.open.mockImplementation(
      () =>
        new Promise<void>(res => {
          resolveOpen = res;
        }),
    );

    const connectPromise = serialManager.connect();

    // Allow requestPort to resolve and enter openWithPort
    await new Promise(resolve => setTimeout(resolve, 0));

    expect(serialManager.getStatus()).toBe('connecting');

    // Call disconnect
    const disconnectPromise = serialManager.disconnect();
    expect(serialManager.getStatus()).toBe('disconnecting');

    // Finish open
    if (resolveOpen) resolveOpen();

    await expect(connectPromise).rejects.toThrow('Connection aborted');
    await disconnectPromise;

    expect(mockPort.close).toHaveBeenCalled();
    expect(serialManager.getStatus()).toBe('disconnected');
  });

  it('handles concurrent disconnect calls safely', async () => {
    await serialManager.connect();

    // Mock close to take some time so we can overlap calls
    mockPort.close.mockImplementation(() => new Promise(res => setTimeout(res, 10)));

    const p1 = serialManager.disconnect();
    const p2 = serialManager.disconnect();

    await Promise.all([p1, p2]);

    expect(mockPort.close).toHaveBeenCalledTimes(1);
    expect(serialManager.getStatus()).toBe('disconnected');
  });
});

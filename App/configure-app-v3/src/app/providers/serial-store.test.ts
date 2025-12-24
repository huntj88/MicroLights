import { describe, it, expect, vi, beforeEach } from 'vitest';

import { serialManager } from './serial-manager';
import { useSerialStore } from './serial-store';

// Define a type for the listener function
type Listener = (...args: unknown[]) => void;

// Mock the serialManager dependency
vi.mock('./serial-manager', () => {
  const listeners: Record<string, Set<Listener> | undefined> = {
    'connection-status': new Set(),
    log: new Set(),
  };

  return {
    serialManager: {
      on: vi.fn((event: string, listener: Listener) => {
        listeners[event] ??= new Set();
        listeners[event].add(listener);
        return () => {
          listeners[event]?.delete(listener);
        };
      }),
      getStatus: vi.fn(() => 'disconnected'),
      isSupported: vi.fn(() => true),
      connect: vi.fn(),
      disconnect: vi.fn(),
      send: vi.fn(),
      // Helper to trigger events for testing
      _emit: (event: string, ...args: unknown[]) => {
        listeners[event]?.forEach(l => {
          l(...args);
        });
      },
    },
  };
});

// Helper type for the mocked manager with _emit
type MockSerialManager = typeof serialManager & {
  _emit: (event: string, ...args: unknown[]) => void;
};

const mockSerialManager = serialManager as unknown as MockSerialManager;

describe('useSerialStore', () => {
  beforeEach(() => {
    // Reset store state
    useSerialStore.setState({
      status: 'disconnected',
      logs: [],
      isSupported: true,
    });
    vi.clearAllMocks();
  });

  it('should initialize with default values', () => {
    const state = useSerialStore.getState();
    expect(state.status).toBe('disconnected');
    expect(state.logs).toEqual([]);
    expect(state.isSupported).toBe(true);
  });

  it('should call serialManager.connect when connect is called', async () => {
    await useSerialStore.getState().connect();
    // eslint-disable-next-line @typescript-eslint/unbound-method
    expect(serialManager.connect).toHaveBeenCalled();
  });

  it('should call serialManager.disconnect when disconnect is called', async () => {
    await useSerialStore.getState().disconnect();
    // eslint-disable-next-line @typescript-eslint/unbound-method
    expect(serialManager.disconnect).toHaveBeenCalled();
  });

  it('should call serialManager.send when send is called', async () => {
    const data = { test: 'data' };
    await useSerialStore.getState().send(data);
    // eslint-disable-next-line @typescript-eslint/unbound-method
    expect(serialManager.send).toHaveBeenCalledWith(data);
  });

  it('should update status when serialManager emits connection-status', () => {
    mockSerialManager._emit('connection-status', 'connected');
    expect(useSerialStore.getState().status).toBe('connected');

    mockSerialManager._emit('connection-status', 'error');
    expect(useSerialStore.getState().status).toBe('error');
  });

  it('should add logs when serialManager emits log', () => {
    const logEntry = {
      id: '1',
      timestamp: 1234567890,
      type: 'rx' as const,
      message: 'test message',
    };

    mockSerialManager._emit('log', logEntry);

    const logs = useSerialStore.getState().logs;
    expect(logs).toHaveLength(1);
    expect(logs[0]).toEqual(logEntry);
  });

  it('should limit logs to MAX_LOGS (500)', () => {
    const MAX_LOGS = 500;
    // Add 501 logs
    for (let i = 0; i < MAX_LOGS + 1; i++) {
      mockSerialManager._emit('log', {
        id: String(i),
        timestamp: Date.now(),
        type: 'rx',
        message: `msg ${String(i)}`,
      });
    }

    const logs = useSerialStore.getState().logs;
    expect(logs).toHaveLength(MAX_LOGS);
    // Should have dropped the first one (id '0').
    // Logs are prepended, so index 0 is the newest (id '500')
    expect(logs[0].id).toBe('500');
    // And the last one is the oldest remaining (id '1')
    expect(logs[MAX_LOGS - 1].id).toBe('1');
  });

  it('should clear logs when clearLogs is called', () => {
    // Add a log
    mockSerialManager._emit('log', {
      id: '1',
      timestamp: 123,
      type: 'rx',
      message: 'test',
    });
    expect(useSerialStore.getState().logs).toHaveLength(1);

    useSerialStore.getState().clearLogs();
    expect(useSerialStore.getState().logs).toHaveLength(0);
  });
});

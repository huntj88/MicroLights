import { create } from 'zustand';

import type { SerialLogEntry } from '@/components/serial-log/SerialLogPanel';

import { serialManager, type ConnectionStatus } from './serial-manager';

export interface SerialStoreState {
  status: ConnectionStatus;
  logs: SerialLogEntry[];
  isSupported: boolean;

  // Actions
  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
  send: (data: unknown) => Promise<void>;
  clearLogs: () => void;
}

const MAX_LOGS = 500;

export const useSerialStore = create<SerialStoreState>(set => {
  // Initialize listeners
  serialManager.on('connection-status', status => {
    set({ status });
  });

  serialManager.on('log', entry => {
    set(state => {
      const newLogs = [entry, ...state.logs];
      if (newLogs.length > MAX_LOGS) {
        newLogs.pop();
      }
      return { logs: newLogs };
    });
  });

  return {
    status: serialManager.getStatus(),
    logs: [],
    isSupported: serialManager.isSupported(),

    connect: async () => {
      await serialManager.connect();
    },

    disconnect: async () => {
      await serialManager.disconnect();
    },

    send: async (data: unknown) => {
      await serialManager.send(data);
    },

    clearLogs: () => {
      set({ logs: [] });
    },
  };
});

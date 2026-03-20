/// <reference types="w3c-web-usb" />
import { type SerialLogEntry } from '@/components/serial-log/SerialLogPanel';

// WebUSB Manager for newline-delimited messages (NDJSON or raw text)
// - Maintains a single connection via vendor-specific USB class
// - Sends strings directly, or stringifies objects
// - Receives line-delimited data over bulk transfers

const USB_VENDOR_ID = 0xcafe;
const USB_CONFIGURATION = 1;
const USB_INTERFACE = 0;
const USB_ENDPOINT_OUT = 1; // EPNUM_VENDOR_OUT (0x01)
const USB_ENDPOINT_IN = 1; // EPNUM_VENDOR_IN (0x81, without direction bit)
const USB_PACKET_SIZE = 64;

export type SerialEventType = 'connection-status' | 'data' | 'log';

export type ConnectionStatus =
  | 'disconnected'
  | 'connecting'
  | 'connected'
  | 'disconnecting'
  | 'error';

export interface SerialEvents {
  'connection-status': (status: ConnectionStatus, error?: unknown) => void;
  data: (line: string, json?: unknown) => void;
  log: (entry: SerialLogEntry) => void;
}

type Listener<K extends keyof SerialEvents> = SerialEvents[K];

class WebUSBManager {
  private device: USBDevice | null = null;
  private abortCtl: AbortController | null = null;
  private _status: ConnectionStatus = 'disconnected';
  private encoder = new TextEncoder();
  private decoder = new TextDecoder();

  private listeners: { [K in keyof SerialEvents]: Set<Listener<K>> } = {
    'connection-status': new Set(),
    data: new Set(),
    log: new Set(),
  };

  isSupported(): boolean {
    return typeof navigator !== 'undefined' && !!navigator.usb;
  }

  getStatus(): ConnectionStatus {
    return this._status;
  }

  on<K extends keyof SerialEvents>(event: K, listener: Listener<K>): () => void {
    this.listeners[event].add(listener);
    return () => this.listeners[event].delete(listener);
  }

  private emit<K extends keyof SerialEvents>(event: K, ...args: Parameters<SerialEvents[K]>) {
    for (const l of this.listeners[event]) {
      try {
        // @ts-expect-error - TS can't infer the variadic param type across union events
        l(...args);
      } catch (e) {
        console.error('Error in USB listener', e);
      }
    }
  }

  private setStatus(status: ConnectionStatus, error?: unknown) {
    this._status = status;
    this.emit('connection-status', status, error);
    if (error) {
      this.log(
        'inbound',
        `Error: ${error instanceof Error ? error.message : JSON.stringify(error)}`,
      );
    }
  }

  private log(direction: 'inbound' | 'outbound', payload: string) {
    const entry: SerialLogEntry = {
      id: crypto.randomUUID(),
      timestamp: new Date().toISOString(),
      direction,
      payload,
    };
    this.emit('log', entry);
  }

  async connect(): Promise<void> {
    if (!this.isSupported()) {
      throw new Error('WebUSB not supported');
    }
    if (this._status === 'connected' || this._status === 'connecting') return;

    this.setStatus('connecting');

    try {
      const device = await navigator.usb.requestDevice({
        filters: [{ vendorId: USB_VENDOR_ID }],
      });
      await this.openDevice(device);
    } catch (err) {
      this.setStatus('disconnected', err);
      throw err;
    }
  }

  private async openDevice(device: USBDevice) {
    try {
      await device.open();

      if (this._status !== 'connecting') {
        await device.close();
        throw new Error('Connection aborted');
      }

      await device.selectConfiguration(USB_CONFIGURATION);
      await device.claimInterface(USB_INTERFACE);

      this.device = device;
      this.abortCtl = new AbortController();

      this.setStatus('connected');
      this.log('inbound', 'Connected to USB device');

      // Start read loop
      this.readLoop(this.abortCtl.signal).catch((err: unknown) => {
        this.setStatus('error', err);
        void this.disconnect();
      });

      // Handle disconnects
      navigator.usb.addEventListener('disconnect', this.handleDisconnect);
    } catch (err) {
      try {
        await device.close();
      } catch {
        /* ignore */
      }
      throw err;
    }
  }

  private handleDisconnect = (e: USBConnectionEvent) => {
    if (e.device === this.device) {
      this.log('inbound', 'Device disconnected');
      void this.disconnect();
    }
  };

  private async readLoop(signal: AbortSignal) {
    if (!this.device) return;

    let buffer = '';

    while (this._status === 'connected' && !signal.aborted) {
      const result = await this.device.transferIn(USB_ENDPOINT_IN, USB_PACKET_SIZE);

      if (result.data && result.data.byteLength > 0) {
        buffer += this.decoder.decode(result.data);

        // Split on newlines and emit complete lines
        const parts = buffer.split(/\r?\n/);
        // Keep the last (possibly incomplete) chunk in the buffer
        buffer = parts[parts.length - 1] ?? '';

        for (let i = 0; i < parts.length - 1; i++) {
          const line = parts[i];
          if (line.length === 0) continue;

          let parsed: unknown;
          try {
            parsed = JSON.parse(line);
          } catch {
            // Not JSON, that's fine
          }

          this.emit('data', line, parsed);
          this.log('inbound', line);
        }
      }
    }
  }

  async send(data: unknown): Promise<void> {
    if (this._status !== 'connected' || !this.device) {
      throw new Error('USB device not connected');
    }

    const isString = typeof data === 'string';
    const payload = isString ? data : JSON.stringify(data);
    const line = payload + '\n';

    await this.device.transferOut(USB_ENDPOINT_OUT, this.encoder.encode(line));

    this.log('outbound', payload);
  }

  async disconnect(): Promise<void> {
    if (this._status === 'disconnected' || this._status === 'disconnecting') return;

    this.setStatus('disconnecting');

    navigator.usb.removeEventListener('disconnect', this.handleDisconnect);

    try {
      this.abortCtl?.abort();
    } catch {
      /* ignore */
    }

    try {
      if (this.device) {
        await this.device.releaseInterface(USB_INTERFACE).catch(() => {
          /* ignore */
        });
        await this.device.close().catch(() => {
          /* ignore */
        });
      }
    } catch {
      /* ignore */
    }
    this.device = null;

    this.setStatus('disconnected');
    this.log('inbound', 'Disconnected');
  }
}

export const serialManager = new WebUSBManager();

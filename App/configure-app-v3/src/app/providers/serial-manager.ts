/// <reference types="w3c-web-serial" />
import { type SerialLogEntry } from '@/components/serial-log/SerialLogPanel';

// Web Serial Manager for newline-delimited messages (NDJSON or raw text)
// - Maintains a single connection
// - Sends strings directly, or stringifies objects
// - Receives line-delimited data

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

class WebSerialManager {
  private port: SerialPort | null = null;
  private reader: ReadableStreamDefaultReader<string> | null = null;
  private writer: WritableStreamDefaultWriter<Uint8Array> | null = null;
  private abortCtl: AbortController | null = null;
  private _status: ConnectionStatus = 'disconnected';

  private listeners: { [K in keyof SerialEvents]: Set<Listener<K>> } = {
    'connection-status': new Set(),
    data: new Set(),
    log: new Set(),
  };

  isSupported(): boolean {
    return typeof navigator !== 'undefined' && !!navigator.serial;
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
        console.error('Error in serial listener', e);
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

  async connect(options?: { baudRate?: number }): Promise<void> {
    if (!this.isSupported()) {
      throw new Error('Web Serial not supported');
    }
    if (this._status === 'connected') return;

    this.setStatus('connecting');

    try {
      // Request a port from the user
      const serialApi = navigator.serial;
      const port: SerialPort = await serialApi.requestPort({});
      await this.openWithPort(port, options);
    } catch (err) {
      this.setStatus('disconnected', err);
      throw err;
    }
  }

  private async openWithPort(port: SerialPort, options?: { baudRate?: number }) {
    try {
      const baudRate = options?.baudRate ?? 115200;
      await port.open({ baudRate });

      if (this._status !== 'connecting') {
        await port.close();
        throw new Error('Connection aborted');
      }

      const writer = port.writable?.getWriter();
      if (!writer) {
        throw new Error('Unable to acquire writable stream');
      }

      // Prepare reader pipeline: Binary -> Text -> Lines
      const decoderStream = new TextDecoderStream();

      const readable = port.readable;
      if (!readable) {
        writer.releaseLock();
        throw new Error('Unable to acquire readable stream');
      }

      this.abortCtl = new AbortController();
      const signal = this.abortCtl.signal;

      class LineSplitterTransformer implements Transformer<string, string> {
        private buffer = '';
        transform(chunk: string, controller: TransformStreamDefaultController<string>) {
          const data = this.buffer + chunk;
          const parts = data.split(/\r?\n/);
          for (let i = 0; i < parts.length - 1; i++) controller.enqueue(parts[i]);
          this.buffer = parts[parts.length - 1] ?? '';
        }
        flush(controller: TransformStreamDefaultController<string>) {
          if (this.buffer.length) controller.enqueue(this.buffer);
          this.buffer = '';
        }
      }
      const lineSplitter = new TransformStream<string, string>(new LineSplitterTransformer());

      const reader = readable
        .pipeThrough(decoderStream as unknown as ReadableWritablePair<string, Uint8Array>)
        .pipeThrough(lineSplitter)
        .getReader();

      this.port = port;
      this.writer = writer;
      this.reader = reader;

      this.setStatus('connected');
      this.log('inbound', 'Connected to serial port');

      // Start read loop
      this.readLoop(signal).catch((err: unknown) => {
        this.setStatus('error', err);
        void this.disconnect();
      });

      // Handle disconnects
      navigator.serial.addEventListener('disconnect', this.handleDisconnect);
    } catch (err) {
      if (this.port?.readable) {
        await this.port.close().catch(() => {
          this.writer?.releaseLock();
        });
      }
      throw err;
    }
  }

  private handleDisconnect = (e: Event) => {
    const tgt = (e as unknown as { target?: unknown }).target ?? null;
    if (tgt === this.port) {
      this.log('inbound', 'Device disconnected');
      void this.disconnect();
    }
  };

  private async readLoop(signal: AbortSignal) {
    if (!this.reader) return;

    try {
      while (this._status === 'connected' && !signal.aborted) {
        const { value, done } = await this.reader.read();
        if (done) break;

        // Try to parse JSON, but always emit raw
        let parsed: unknown;
        try {
          parsed = JSON.parse(value);
        } catch {
          // Not JSON, that's fine
        }

        this.emit('data', value, parsed);
        this.log('inbound', value);
      }
    } finally {
      this.reader.releaseLock();
    }
  }

  async send(data: unknown): Promise<void> {
    if (this._status !== 'connected' || !this.writer) {
      throw new Error('Serial port not connected');
    }

    const isString = typeof data === 'string';
    const payload = isString ? data : JSON.stringify(data);
    const line = payload + '\n';

    const encoder = new TextEncoder();
    await this.writer.write(encoder.encode(line));

    this.log('outbound', payload);
  }

  async disconnect(): Promise<void> {
    if (this._status === 'disconnected' || this._status === 'disconnecting') return;

    this.setStatus('disconnecting');

    navigator.serial.removeEventListener('disconnect', this.handleDisconnect);

    try {
      this.abortCtl?.abort();
    } catch {
      /* ignore */
    }

    try {
      if (this.reader) {
        await this.reader.cancel().catch(() => {
          /* ignore */
        });
        this.reader.releaseLock();
      }
    } catch {
      /* ignore */
    }
    this.reader = null;

    try {
      if (this.writer) {
        await this.writer.close().catch(() => {
          /* ignore */
        });
        this.writer.releaseLock();
      }
    } catch {
      /* ignore */
    }
    this.writer = null;

    try {
      await this.port?.close().catch(() => {
        /* ignore */
      });
    } catch {
      /* ignore */
    }
    this.port = null;

    this.setStatus('disconnected');
    this.log('inbound', 'Disconnected');
  }
}

export const serialManager = new WebSerialManager();

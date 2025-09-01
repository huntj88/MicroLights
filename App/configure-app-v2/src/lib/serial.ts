// Web Serial Manager for newline-delimited JSON (NDJSON)
// - Maintains a single connection
// - Sends minified JSON followed by a single "\n"
// - Receives line-delimited JSON and parses each line separately

export type NDJSONMessage = unknown;

export type SerialEvents = {
  open: () => void;
  close: () => void;
  error: (err: unknown) => void;
  message: (msg: NDJSONMessage) => void;
  raw: (line: string) => void; // fired for each received line (even if JSON parse fails)
  log: (entry: SerialLogEntry) => void; // fired whenever a log entry is added
};

type Listener<K extends keyof SerialEvents> = SerialEvents[K];

class WebSerialManager {
  private port: SerialPort | null = null;
  private reader: ReadableStreamDefaultReader<string> | null = null;
  private writer: WritableStreamDefaultWriter<Uint8Array> | null = null;
  private abortCtl: AbortController | null = null;
  private _isOpen = false;
  private _log: SerialLogEntry[] = [];
  private _logSeq = 0;
  private static MAX_LOG = 500;
  private listeners: { [K in keyof SerialEvents]: Set<Listener<K>> } = {
    open: new Set(),
    close: new Set(),
    error: new Set(),
    message: new Set(),
    raw: new Set(),
    log: new Set(),
  };

  isSupported(): boolean {
    return typeof navigator !== 'undefined' && !!navigator.serial;
  }

  isOpen(): boolean {
    return this._isOpen;
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
        // Swallow listener errors but surface to error channel
        if (event !== 'error') this.emit('error', e);
      }
    }
  }

  async requestAndOpen(options?: { baudRate?: number }): Promise<void> {
    if (!this.isSupported()) {
      throw new Error('Web Serial not supported');
    }
    if (this._isOpen) return; // already open

    // Request a port from the user
    const serialApi = navigator.serial!;
    const port: SerialPort = await serialApi.requestPort({});
    await this.openWithPort(port, options);
  }

  async openWithExistingPort(): Promise<boolean> {
    // Attempts to open the first previously authorized port (no prompt)
    if (!this.isSupported()) return false;
    if (this._isOpen) return true;
    const ports: SerialPort[] = await navigator.serial!.getPorts();
    if (!ports.length) return false;
    await this.openWithPort(ports[0]!);
    return true;
  }

  private async openWithPort(port: SerialPort, options?: { baudRate?: number }) {
    // Open the port
    const baudRate = options?.baudRate ?? 115200;
    await port.open({ baudRate });

    // Prepare writer using TextEncoder
    const writer = port.writable?.getWriter();
    if (!writer) {
      await port.close();
      throw new Error('Unable to acquire writable stream for serial port');
    }

    // Prepare reader pipeline: Binary -> Text -> Lines
    const decoder = new TextDecoder();
    const decoderStream = new TransformStream<Uint8Array, string>({
      transform(chunk, controller) {
        const text = decoder.decode(chunk, { stream: true });
        if (text.length) controller.enqueue(text);
      },
      flush(controller) {
        const tail = decoder.decode();
        if (tail.length) controller.enqueue(tail);
      },
    });
    const readable = port.readable;
    if (!readable) {
      writer.releaseLock();
      await port.close();
      throw new Error('Unable to acquire readable stream for serial port');
    }
    this.abortCtl = new AbortController();
    const signal = this.abortCtl.signal;
    let buffer = '';
    const lineSplitter = new TransformStream<string, string>({
      transform(chunk, controller) {
        const data = buffer + chunk;
        const parts = data.split(/\r?\n/);
        for (let i = 0; i < parts.length - 1; i++) controller.enqueue(parts[i]!);
        buffer = parts[parts.length - 1] ?? '';
      },
      flush(controller) {
        if (buffer.length) controller.enqueue(buffer);
        buffer = '';
      },
    });

    // Pipe the port readable to text decoder then to line splitter
    const reader = readable.pipeThrough(decoderStream).pipeThrough(lineSplitter).getReader();

    // Save references
    this.port = port;
    this.writer = writer;
    this.reader = reader;
    this._isOpen = true;
    this.emit('open');

    // Start read loop
    this.readLoop(signal).catch(err => this.emit('error', err));

    // Handle OS-level disconnect events to update state
    navigator.serial!.addEventListener('disconnect', (e: Event) => {
      const tgt = (e as unknown as { target?: unknown })?.target ?? null;
      if (tgt === this.port) {
        // Close state if this is our port
        this.close().catch(() => {
          /* ignore */
        });
      }
    });
  }

  private async readLoop(signal: AbortSignal) {
    if (!this.reader) return;
    try {
      while (this._isOpen && !signal.aborted) {
        const { value, done } = await this.reader.read();
        if (done) break;
        if (value == null) continue;
        this.emit('raw', value);
        // Try to parse JSON per line
        try {
          const parsed = JSON.parse(value);
          this.emit('message', parsed);
          this.addLog({ dir: 'rx', raw: value, ok: true, json: parsed });
        } catch (e) {
          // Non-JSON or parse error, surface on error channel as well
          this.emit('error', e);
          const msg = e instanceof Error ? e.message : String(e);
          this.addLog({ dir: 'rx', raw: value, ok: false, error: msg });
        }
      }
    } finally {
      // Exit
      try {
        this.reader?.releaseLock();
      } catch {
        /* ignore */
      }
    }
  }

  async sendJSON(obj: unknown): Promise<void> {
    if (!this._isOpen || !this.writer) throw new Error('Serial port not open');
    const encoder = new TextEncoder();
    // Minified JSON string followed by newline
    const line = JSON.stringify(obj) + '\n';
    await this.writer.write(encoder.encode(line));
    this.addLog({ dir: 'tx', raw: line.trimEnd(), ok: true, json: obj });
  }

  async close(): Promise<void> {
    if (!this._isOpen) return;
    this._isOpen = false;
    try {
      this.abortCtl?.abort();
    } catch {
      /* ignore */
    }
    try {
      if (this.reader) {
        try {
          await this.reader.cancel();
        } catch {
          /* ignore */
        }
        this.reader.releaseLock();
      }
    } catch {
      /* ignore */
    } finally {
      this.reader = null;
    }
    try {
      if (this.writer) {
        await this.writer.close();
        this.writer.releaseLock();
      }
    } catch {
      /* ignore */
    } finally {
      this.writer = null;
    }
    try {
      await this.port?.close();
    } catch {
      /* ignore */
    } finally {
      this.port = null;
      this.emit('close');
    }
  }

  // ---- Log API ----
  getLog(): ReadonlyArray<SerialLogEntry> {
    return this._log;
  }
  clearLog() {
    this._log = [];
    this._logSeq = 0;
  }
  private addLog(partial: Omit<SerialLogEntry, 'id' | 'ts'>) {
    const entry: SerialLogEntry = {
      id: ++this._logSeq,
      ts: Date.now(),
      ...partial,
    };
    this._log.push(entry);
    if (this._log.length > WebSerialManager.MAX_LOG) this._log.shift();
    this.emit('log', entry);
  }
}

// Export a singleton for app-wide use
export const serial = new WebSerialManager();

export type { WebSerialManager };

export type SerialLogEntry = {
  id: number;
  ts: number; // epoch ms
  dir: 'rx' | 'tx';
  raw: string; // the line as seen over the wire (without trailing newline)
  ok: boolean;
  json?: unknown; // present when ok === true
  error?: string; // present when ok === false
};

// Minimal Web Serial type declarations for TypeScript
// See: https://wicg.github.io/serial/

declare global {
  interface SerialPort {
    readable: ReadableStream<Uint8Array> | null;
    writable: WritableStream<Uint8Array> | null;
    open(options: { baudRate: number }): Promise<void>;
    close(): Promise<void>;
  }

  interface SerialPortFilter {
    usbVendorId?: number;
    usbProductId?: number;
  }

  interface SerialPortRequestOptions {
    filters?: SerialPortFilter[];
  }

  interface Serial {
    getPorts(): Promise<SerialPort[]>;
    requestPort(options?: SerialPortRequestOptions): Promise<SerialPort>;
    addEventListener(type: 'connect' | 'disconnect', listener: (e: Event) => void): void;
    removeEventListener(type: 'connect' | 'disconnect', listener: (e: Event) => void): void;
  }

  interface Navigator {
    serial?: Serial;
  }
}

export {};

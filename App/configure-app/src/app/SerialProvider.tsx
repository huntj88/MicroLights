import {
  createContext,
  PropsWithChildren,
  useContext,
  useEffect,
  useRef,
  useState,
} from "react";

// Modified from from this gist
// https://gist.github.com/joshpensky/426d758c5779ac641d1d09f9f5894153

// RESOURCES:
// https://web.dev/serial/
// https://reillyeon.github.io/serial/#onconnect-attribute-0
// https://codelabs.developers.google.com/codelabs/web-serial

export type PortState = "closed" | "closing" | "open" | "opening";

export type SerialMessage = {
  value: string;
  timestamp: number;
};

type SerialMessageCallback = (message: SerialMessage) => void;

export interface SerialContextValue {
  canUseSerial: boolean;
  hasTriedAutoconnect: boolean;
  portState: PortState;
  connect(): Promise<boolean>;
  disconnect(): void;
  send(message: string): Promise<boolean>;
  subscribe(callback: SerialMessageCallback): () => void;
}
export const SerialContext = createContext<SerialContextValue>({
  canUseSerial: false,
  hasTriedAutoconnect: false,
  connect: () => Promise.resolve(false),
  disconnect: () => {},
  portState: "closed",
  send: () => Promise.resolve(false),
  subscribe: () => () => {},
});

export const useSerial = () => useContext(SerialContext);

type SerialProviderProps = object;
const SerialProvider = ({
  children,
}: PropsWithChildren<SerialProviderProps>) => {
  const [canUseSerial] = useState(() => navigator.serial !== undefined);

  const [portState, setPortState] = useState<PortState>("closed");
  const [hasTriedAutoconnect, setHasTriedAutoconnect] = useState(false);
  const [hasManuallyDisconnected, setHasManuallyDisconnected] = useState(false);

  const portRef = useRef<SerialPort | null>(null);
  const readerRef = useRef<ReadableStreamDefaultReader | null>(null);
  const readerClosedPromiseRef = useRef<Promise<void>>(Promise.resolve());

  const currentSubscriberIdRef = useRef<number>(0);
  const subscribersRef = useRef<Map<number, SerialMessageCallback>>(new Map());

  /**
   * Send a message to the connected port
   *
   * @param message string to send
   * @returns a boolean to know if the process completed or not
   */
  const send = async (message: string) => {
    if (portState === "open" && portRef.current?.writable) {
      let sent = false;
      const writer = portRef.current.writable.getWriter();
      const data = new TextEncoder().encode(message);
      try {
        await writer.write(data);
        sent = true;
      } catch (error) {
        console.error("Failed to send message", error);
      } finally {
        writer.releaseLock();
      }
      return sent;
    } else {
      console.error("Cannot send message: Port is not open");
      return false;
    }
  };

  /**
   * Subscribes a callback function to the message event.
   *
   * @param callback the callback function to subscribe
   * @returns an unsubscribe function
   */
  const subscribe = (callback: SerialMessageCallback) => {
    const id = currentSubscriberIdRef.current;
    subscribersRef.current.set(id, callback);
    currentSubscriberIdRef.current++;

    return () => {
      subscribersRef.current.delete(id);
    };
  };

  /**
   * Reads from the given port until it's been closed.
   *
   * @param port the port to read from
   */
  const readUntilClosed = async (port: SerialPort) => {
    if (port.readable) {
      const textDecoder = new TextDecoderStream();
      const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
      readerRef.current = textDecoder.readable.getReader();

      try {
        // eslint-disable-next-line no-constant-condition
        while (true) {
          const { value, done } = await readerRef.current.read();
          if (done) {
            break;
          }
          const timestamp = Date.now();
          Array.from(subscribersRef.current).forEach(([name, callback]) => {
            callback({ value, timestamp });
          });
        }
      } catch (error) {
        console.error(error);
      } finally {
        readerRef.current.releaseLock();
      }

      await readableStreamClosed.catch(() => {}); // Ignore the error
    }
  };

  /**
   * Attempts to open the given port.
   */
  const openPort = async (port: SerialPort) => {
    try {
      await port.open({ baudRate: 115200 });
      portRef.current = port;
      setPortState("open");
      setHasManuallyDisconnected(false);
    } catch (error) {
      console.error(error);
      setPortState("closed");
      console.error("Could not open port");
    }
  };

  const manualConnectToPort = async () => {
    if (canUseSerial && portState === "closed") {
      setPortState("opening");
      // TODO: Add filters to requestPort

      //   const filters = [
      //     // Can identify the vendor and product IDs by plugging in the device and visiting: chrome://device-log/
      //     // the IDs will be labeled `vid` and `pid`, respectively
      //     {
      //       usbVendorId: 0x1a86,
      //       usbProductId: 0x7523,
      //     },
      //   ];
      try {
        // const port = await navigator.serial.requestPort({ filters });
        const port = await navigator.serial.requestPort();
        await openPort(port);
        return true;
      } catch (error) {
        console.error(error);
        setPortState("closed");
        console.error("User did not select port");
      }
    }
    return false;
  };

  const autoConnectToPort = async () => {
    if (canUseSerial && portState === "closed") {
      setPortState("opening");
      const availablePorts = await navigator.serial.getPorts();
      if (availablePorts.length) {
        const port = availablePorts[0];
        await openPort(port);
        return true;
      } else {
        setPortState("closed");
      }
      setHasTriedAutoconnect(true);
    }
    return false;
  };

  const manualDisconnectFromPort = async () => {
    if (canUseSerial && portState === "open") {
      const port = portRef.current;
      if (port) {
        setPortState("closing");

        // Cancel any reading from port
        readerRef.current?.cancel();
        await readerClosedPromiseRef.current;
        readerRef.current = null;

        // Close and nullify the port
        await port.close();
        portRef.current = null;

        // Update port state
        setHasManuallyDisconnected(true);
        setHasTriedAutoconnect(false);
        setPortState("closed");
      }
    }
  };

  /**
   * Event handler for when the port is disconnected unexpectedly.
   */
  const onPortDisconnect = async () => {
    // Wait for the reader to finish it's current loop
    await readerClosedPromiseRef.current;
    // Update state
    readerRef.current = null;
    readerClosedPromiseRef.current = Promise.resolve();
    portRef.current = null;
    setHasTriedAutoconnect(false);
    setPortState("closed");
  };

  // Handles attaching the reader and disconnect listener when the port is open
  useEffect(() => {
    const port = portRef.current;
    if (portState === "open" && port) {
      // When the port is open, read until closed
      const aborted = { current: false };
      readerRef.current?.cancel();
      readerClosedPromiseRef.current.then(() => {
        if (!aborted.current) {
          readerRef.current = null;
          readerClosedPromiseRef.current = readUntilClosed(port);
        }
      });

      // Attach a listener for when the device is disconnected
      navigator.serial.addEventListener("disconnect", onPortDisconnect);

      return () => {
        aborted.current = true;
        navigator.serial.removeEventListener("disconnect", onPortDisconnect);
      };
    }
  }, [portState]);

  // Tries to auto-connect to a port, if possible
  useEffect(() => {
    if (
      canUseSerial &&
      !hasManuallyDisconnected &&
      !hasTriedAutoconnect &&
      portState === "closed"
    ) {
      //   autoConnectToPort();
    }
  }, [canUseSerial, hasManuallyDisconnected, hasTriedAutoconnect, portState]);

  return (
    <SerialContext.Provider
      value={{
        canUseSerial,
        hasTriedAutoconnect,
        send,
        subscribe,
        portState,
        connect: manualConnectToPort,
        disconnect: manualDisconnectFromPort,
      }}
    >
      {children}
    </SerialContext.Provider>
  );
};

export default SerialProvider;

import { useEffect, useRef } from 'react';

import { serial, type NDJSONMessage } from './serial';

export function useSerial(
  onMessage?: (msg: NDJSONMessage) => void,
  onError?: (err: unknown) => void,
) {
  const msgRef = useRef(onMessage);
  const errRef = useRef(onError);
  useEffect(() => {
    msgRef.current = onMessage;
  }, [onMessage]);
  useEffect(() => {
    errRef.current = onError;
  }, [onError]);

  useEffect(() => {
    const offMsg = serial.on('message', m => msgRef.current?.(m));
    const offErr = serial.on('error', e => errRef.current?.(e));
    return () => {
      offMsg();
      offErr();
    };
  }, []);

  return {
    isSupported: serial.isSupported(),
    isOpen: serial.isOpen(),
    connect: (baudRate?: number) => serial.requestAndOpen({ baudRate }),
    disconnect: () => serial.close(),
    sendJSON: (payload: unknown) => serial.sendJSON(payload),
  } as const;
}

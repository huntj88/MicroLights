"use client";
import { useEffect, useRef, useState } from "react";
import SetConfigCard, { ChipSetConfig } from "./SetConfigCard";
import { Button } from "@fluentui/react-components";
import { useSerial } from "@/app/SerialProvider";

export default function Create() {
  const serial = useSerial();

  const buffer = useRef<string>("");
  const subscription = useRef<() => void>(() => {});
  useEffect(() => {
    if (serial.portState === "open") {
      subscription.current = serial.subscribe((message) => {
        buffer.current += message.value;

        try {
          const json = JSON.parse(
            buffer.current.substring(buffer.current.indexOf("\n{")),
          );
          console.log("json", json);
          buffer.current = ""; // Clear buffer after successful parsing
        } catch (e) {
          console.warn(e);
          // Wait for more data if JSON is incomplete
        }
      });
    } else {
      subscription.current(); //unsubscribe
      subscription.current = () => {};
      buffer.current = "";
    }

    return () => {
      subscription.current();
      subscription.current = () => {};
      buffer.current = "";
    };
  }, [serial.portState]);

  const [chipSetConfig, setChipSetConfig] = useState<ChipSetConfig>({
    name: "test",
    parts: [
      {
        waveConfigName: undefined,
        fingers: [],
        caseColor: "#6699ff",
      },
      {
        waveConfigName: undefined,
        fingers: [],
        caseColor: "#000000",
      },
    ],
  });
  return (
    <div
      style={{
        display: "flex",
        width: "100%",
        padding: "16px",
      }}
    >
      <SetConfigCard
        chipSetConfig={chipSetConfig}
        partIndex={0}
        updateChipSetConfig={setChipSetConfig}
      />
      <SetConfigCard
        chipSetConfig={chipSetConfig}
        partIndex={1}
        updateChipSetConfig={setChipSetConfig}
      />
      <Button
        onClick={() => {
          serial.connect().then((connected) => {
            if (connected) {
              console.log("Connected to serial port");
            } else {
              console.log("Failed to connect to serial port");
            }
          });
        }}
      >
        connect
      </Button>
      <Button
        onClick={() => {
          const object = {
            command: "setMode",
            index: 1,
            mode: {
              name: "test",
              totalTicks: 10,
              changeAt: [
                {
                  tick: 0,
                  output: "high",
                },
                {
                  tick: 5,
                  output: "low",
                },
              ],
            },
          };
          serial.send(JSON.stringify(object)).then((sent) => {
            if (sent) {
              console.log("Message sent successfully");
            } else {
              console.log("Failed to send message");
            }
          });
        }}
      >
        send
      </Button>
    </div>
  );
}

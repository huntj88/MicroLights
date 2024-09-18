"use client";
import { Text, Textarea } from "@fluentui/react-components";
import { BulbConfig, WaveForm } from "./WaveForm";
import { firstThenAllBulbConfig } from "../config";
import { useEffect, useState } from "react";

export default function Create() {
  const [json, setJson] = useState<string>(
    JSON.stringify(firstThenAllBulbConfig, null, 2),
  );
  const [config, setConfig] = useState<BulbConfig>(firstThenAllBulbConfig);

  useEffect(() => {
    try {
      setConfig(JSON.parse(json));
    } catch (e) {
      // TODO: set error state
    }
  }, [json]);

  return (
    <div style={{ display: "flex", flexDirection: "column", width: "100%" }}>
      <Text>Json Config</Text>
      <Textarea
        contentEditable
        style={{ width: 200, height: 250 }}
        value={json}
        onChange={(_, data) => {
          setJson(data.value);
        }}
      />
      <WaveForm bulbconfig={config} />
    </div>
  );
}

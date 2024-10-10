"use client";
import { Text, Textarea, TextareaOnChangeData } from "@fluentui/react-components";
import { BulbConfig, WaveForm } from "./WaveForm";
import { firstThenAllBulbConfig } from "../config";
import { useCallback, useEffect, useState } from "react";

export default function Create() {
  const [json, setJson] = useState<string>(
    JSON.stringify(firstThenAllBulbConfig, null, 2),
  );
  const [config, setConfig] = useState<BulbConfig>(firstThenAllBulbConfig);
  const [recentlyChanged, setRecentlyChanged] = useState<"config" | "json">("config")

  useEffect(() => {
    try {
      const configJson = JSON.stringify(config, undefined, 2);
      if (configJson !== json) {
        if (recentlyChanged === "json") {
          setConfig(JSON.parse(json));
        } else {
          setJson(configJson)
        } 
      }
    } catch (e) {
      // TODO: set error state
    }
  }, [
    config,
    json,
    recentlyChanged
  ]);

  const onUpdateConfig = useCallback((bulbConfig: BulbConfig) => {
    setConfig(bulbConfig)
    setRecentlyChanged("config")
  }, [setConfig, setRecentlyChanged])

  const onJsonChange = useCallback((ev: React.ChangeEvent<HTMLTextAreaElement>, data: TextareaOnChangeData) => {
    setJson(data.value);
    setRecentlyChanged("json")
  }, [setJson, setRecentlyChanged])

  return (
    <div style={{ display: "flex", flexDirection: "column", width: "100%" }}>
      <Text>Json Config</Text>
      <Textarea
        contentEditable
        style={{ width: 200, height: 250 }}
        value={json}
        onChange={onJsonChange}
      />
      <WaveForm bulbconfig={config} updateConfig={onUpdateConfig} />
    </div>
  );
}

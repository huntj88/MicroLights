"use client";
import {
  Button,
  Text,
  Textarea,
  TextareaOnChangeData,
} from "@fluentui/react-components";
import { useCallback, useEffect, useState } from "react";
import dynamic from "next/dynamic";
import { LogicLevelChange, WaveForm, WaveFormConfig } from "./WaveForm";
import { flashyBulbConfig } from "@/app/config";
import { waveFormPrefix } from "@/app/constants";
import { getFromLocalStorage } from "@/app/useLocalStorage";

export interface WaveFormEditWrapperProps {
  name: string;
}

// dyanmic import to prevent server side rendering, localStorage is not available on the server
export const WaveFormEditWrapper: React.ComponentType<WaveFormEditWrapperProps> =
  dynamic(
    () =>
      import("./WaveFormEditWrapper").then(
        (x) => x.WaveFormEditWrapperInternalLocalStorage,
      ),
    { ssr: false },
  );

export const WaveFormEditWrapperInternalLocalStorage = (props: {
  name: string;
}) => {
  const { name } = props;
  const configFromStorage = getFromLocalStorage<WaveFormConfig>(
    `${waveFormPrefix}-${name}`,
  );

  const [config, setConfig] = useState<WaveFormConfig | undefined>(
    configFromStorage,
  );

  const defaultWithName = {
    ...flashyBulbConfig,
    name,
  };
  return (
    <WaveFormEditWrapperInternal
      storedConfig={config || defaultWithName}
      onSaveConfig={(config) => {
        localStorage.setItem(
          `${waveFormPrefix}${config.name}`,
          JSON.stringify(config),
        );
        setConfig(config);
      }}
    />
  );
};

export interface WaveFormEditWrapperInternalProps {
  storedConfig: WaveFormConfig;
  onSaveConfig: (config: WaveFormConfig) => void;
}

export const WaveFormEditWrapperInternal: React.FC<
  WaveFormEditWrapperInternalProps
> = (props: WaveFormEditWrapperInternalProps) => {
  const { storedConfig, onSaveConfig } = props;
  const [json, setJson] = useState<string>(
    JSON.stringify(storedConfig, null, 2),
  );
  const [config, setConfig] = useState<WaveFormConfig>(storedConfig);
  const [recentlyChanged, setRecentlyChanged] = useState<"config" | "json">(
    "config",
  );

  useEffect(() => {
    try {
      const configJson = JSON.stringify(config, undefined, 2);
      if (configJson !== json) {
        if (recentlyChanged === "json") {
          setConfig(JSON.parse(json));
        } else {
          setJson(configJson);
        }
      }
    } catch (e) {
      console.error(e);
      // TODO: set error state
    }
  }, [config, json, recentlyChanged]);

  const onUpdateConfig = useCallback(
    (bulbConfig: WaveFormConfig) => {
      setConfig(bulbConfig);
      setRecentlyChanged("config");
    },
    [setConfig, setRecentlyChanged],
  );

  const onJsonChange = useCallback(
    (
      ev: React.ChangeEvent<HTMLTextAreaElement>,
      data: TextareaOnChangeData,
    ) => {
      setJson(data.value);
      setRecentlyChanged("json");
    },
    [setJson, setRecentlyChanged],
  );

  const onAddMarker = useCallback(() => {
    const copy = { ...config };
    const lastIndex = copy.changeAt.length - 1;
    const last = copy.changeAt[lastIndex];
    const newChange: LogicLevelChange = {
      tick: last ? last.tick + 1 : 0,
      output: "high",
    };
    copy.changeAt = copy.changeAt.concat([newChange]);
    copy.totalTicks = Math.max(copy.totalTicks, newChange.tick);
    onUpdateConfig(copy);
  }, [config, onUpdateConfig]);

  const onRemoveMarker = useCallback(() => {
    const copy = { ...config };
    copy.changeAt = copy.changeAt.slice(0, -1);
    onUpdateConfig(copy);
  }, [config, onUpdateConfig]);

  const onSaveWaveFormConfig = useCallback(() => {
    onSaveConfig(config);
  }, [config, onSaveConfig]);

  const nameRequired = config.name === "";
  const saveDisabled = config === storedConfig || nameRequired;

  return (
    <div style={{ display: "flex", flexDirection: "column", width: "100%" }}>
      <WaveForm config={config} updateConfig={onUpdateConfig} />
      <Button onClick={onAddMarker}>Add Marker to end</Button>
      <Button onClick={onRemoveMarker}>Remove Marker from end</Button>
      <Button disabled={saveDisabled} onClick={onSaveWaveFormConfig}>
        Save Wave Form
      </Button>
      {nameRequired && <Text style={{ color: "red" }}>Name must be set</Text>}
      <Text>Json Config</Text>
      <Textarea
        contentEditable
        style={{ width: 200, height: 250 }}
        value={json}
        onChange={onJsonChange}
      />
    </div>
  );
};

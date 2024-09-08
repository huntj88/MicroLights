"use client";
import { Text, Textarea } from "@fluentui/react-components";

// TODO: add typing
const defaultBulbConfig: unknown = {
  type: "bulb",
  rules: [],
};
export default function Home() {
  return (
    <div style={{ display: "flex", flexDirection: "column" }}>
      <Text>Json Config</Text>
      <Textarea>{JSON.stringify(defaultBulbConfig)}</Textarea>
    </div>
  );
}

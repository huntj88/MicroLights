"use client";
import * as React from "react";
import { Virtualizer } from "@fluentui/react-components/unstable";
import { makeStyles } from "@fluentui/react-components";
import {
  firstColorBulbConfig,
  firstThenAllBulbConfig,
  flashyBulbConfig,
} from "../config";
import { WaveForm } from "@/components/wave/WaveForm";

const useStyles = makeStyles({
  container: {
    display: "flex",
    flexDirection: "column",
    overflowAnchor: "none",
    overflowY: "auto",
    height: "100%",
    paddingLeft: "16px",
    paddingRight: "16px",
    paddingTop: "16px",
    paddingBottom: "16px",
  },
  child: {
    width: "100%",
    paddingLeft: "8px",
    paddingRight: "8px",
    paddingTop: "8px",
    paddingBottom: "8px",
  },
});

export default function Browse() {
  const styles = useStyles();

  const modes = [
    flashyBulbConfig,
    firstThenAllBulbConfig,
    firstColorBulbConfig,
  ];

  return (
    <div aria-label="Browse Modes" className={styles.container} role={"list"}>
      <Virtualizer
        numItems={modes.length}
        virtualizerLength={100}
        itemSize={100}
      >
        {(index) => {
          return (
            <span
              role={"listitem"}
              aria-posinset={index}
              aria-setsize={modes.length}
              key={`test-virtualizer-child-${index}`}
              className={styles.child}
            >
              <WaveForm
                config={modes[index]}
                updateConfig={() => {
                  /* TODO */
                }}
              />
            </span>
          );
        }}
      </Virtualizer>
    </div>
  );
}

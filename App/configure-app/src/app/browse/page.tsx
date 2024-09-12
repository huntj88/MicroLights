"use client";
import * as React from "react";
import { Virtualizer } from "@fluentui/react-components/unstable";
import { makeStyles } from "@fluentui/react-components";
import {
  firstColorBulbConfig,
  firstThenAllBulbConfig,
  flashyBulbConfig,
} from "../config";
import { WaveForm } from "../create/WaveForm";

const useStyles = makeStyles({
  container: {
    display: "flex",
    flexDirection: "column",
    overflowAnchor: "none",
    overflowY: "auto",
    width: "100%",
    height: "100%",
  },
  child: {
    height: "100px",
    lineHeight: "100px",
    width: "100%",
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
              <WaveForm bulbconfig={modes[index]} />
            </span>
          );
        }}
      </Virtualizer>
    </div>
  );
}

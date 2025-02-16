import * as React from "react";
import {
  Dropdown,
  DropdownProps,
  makeStyles,
  Option,
  useId,
} from "@fluentui/react-components";
import { useEffect, useState } from "react";

const useStyles = makeStyles({
  root: {
    // Stack the label above the field with a gap
    display: "grid",
    gridTemplateRows: "repeat(1fr)",
    justifyItems: "start",
    gap: "2px",
    maxWidth: "400px",
  },
});

export const WaveFormDropdown = (props: Partial<DropdownProps>) => {
  const styles = useStyles();
  const dropdownId = useId("dropdown-waveForm");

  const [options, setOptions] = useState<string[]>([]);
  useEffect(() => {
    const availableWaveForms = Object.keys(localStorage)
      .filter(key => key.includes(waveFormPrefix))
      .map(key => key.replace(waveFormPrefix, ""));

    setOptions(availableWaveForms)
  }, []);

  return (
    <div className={styles.root}>
      <label htmlFor={dropdownId}>Config for finger</label>
      <Dropdown inlinePopup id={dropdownId} placeholder="Select WaveForm" {...props}>
        {options.map((option) => (
          <Option key={option}>
            {option}
          </Option>
        ))}
      </Dropdown>
    </div>
  );
};
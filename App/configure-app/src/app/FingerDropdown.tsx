import * as React from "react";
import {
  Dropdown,
  makeStyles,
  Option,
  useId,
} from "@fluentui/react-components";
import type { DropdownProps } from "@fluentui/react-components";

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

export const FingerDropdown = (props: Partial<DropdownProps>) => {
  const dropdownId = useId("dropdown-finger");
  const options = [
    "Left Pinky",
    "Left Index",
    "Left Middle",
    "Left Pointer",
    "Left Thumb",
    "Right Thumb",
    "Right Pointer",
    "Right Middle",
    "Right Index",
    "Right Pinky",
  ];
  const styles = useStyles();
  return (
    <div className={styles.root}>
      <label htmlFor={dropdownId}>Config for finger</label>
      <Dropdown
        inlinePopup
        id={dropdownId}
        placeholder="Select finger"
        {...props}
      >
        {options.map((option) => (
          <Option key={option}>{option}</Option>
        ))}
      </Dropdown>
    </div>
  );
};

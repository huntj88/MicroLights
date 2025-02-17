import * as React from "react";
import { Checkbox } from "@fluentui/react-components";
import { FlexRow } from "./flex/FlexRow";
import { FlexColumn } from "./flex/FlexColumn";

export const FingerCheckboxArea = () => {
  const [leftPinky, setLeftPinky] = React.useState(false);
  const [leftRing, setLeftRing] = React.useState(true);
  const [leftMiddle, setLeftMiddle] = React.useState(false);
  const [leftIndex, setLeftIndex] = React.useState(false);
  const [leftThumb, setLeftThumb] = React.useState(false);
  const [rightThumb, setRightThumb] = React.useState(false);
  const [rightIndex, setRightIndex] = React.useState(false);
  const [rightMiddle, setRightMiddle] = React.useState(false);
  const [rightRing, setRightRing] = React.useState(false);
  const [rightPinky, setRightPinky] = React.useState(false);

  const allChecked =
    leftPinky &&
    leftRing &&
    leftMiddle &&
    leftIndex &&
    leftThumb &&
    rightThumb &&
    rightIndex &&
    rightMiddle &&
    rightRing &&
    rightPinky;

  const anyChecked =
    leftPinky ||
    leftRing ||
    leftMiddle ||
    leftIndex ||
    leftThumb ||
    rightThumb ||
    rightIndex ||
    rightMiddle ||
    rightRing ||
    rightPinky;

  return (
    <>
      <Checkbox
        checked={allChecked ? true : !anyChecked ? false : "mixed"}
        onChange={(_ev, data) => {
          const checked = !!data.checked;
          setLeftPinky(checked);
          setLeftRing(checked);
          setLeftMiddle(checked);
          setLeftIndex(checked);
          setLeftThumb(checked);
          setRightThumb(checked);
          setRightIndex(checked);
          setRightMiddle(checked);
          setRightRing(checked);
          setRightPinky(checked);
        }}
        label="All options"
      />
      <FlexRow>
        <FlexColumn>
          <Checkbox
            checked={leftPinky}
            onChange={() => setLeftPinky((checked) => !checked)}
            label="Left Pinky"
          />
          <Checkbox
            checked={leftRing}
            onChange={() => setLeftRing((checked) => !checked)}
            label="Left Ring"
          />
          <Checkbox
            checked={leftMiddle}
            onChange={() => setLeftMiddle((checked) => !checked)}
            label="Left Middle"
          />
          <Checkbox
            checked={leftIndex}
            onChange={() => setLeftIndex((checked) => !checked)}
            label="Left Index"
          />
          <Checkbox
            checked={leftThumb}
            onChange={() => setLeftThumb((checked) => !checked)}
            label="Left Thumb"
          />
        </FlexColumn>
        <FlexColumn>
          <Checkbox
            checked={rightPinky}
            onChange={() => setRightPinky((checked) => !checked)}
            label="Right Pinky"
          />
          <Checkbox
            checked={rightRing}
            onChange={() => setRightRing((checked) => !checked)}
            label="Right Ring"
          />
          <Checkbox
            checked={rightMiddle}
            onChange={() => setRightMiddle((checked) => !checked)}
            label="Right Middle"
          />
          <Checkbox
            checked={rightIndex}
            onChange={() => setRightIndex((checked) => !checked)}
            label="Right Index"
          />
          <Checkbox
            checked={rightThumb}
            onChange={() => setRightThumb((checked) => !checked)}
            label="Right Thumb"
          />
        </FlexColumn>
      </FlexRow>
    </>
  );
};

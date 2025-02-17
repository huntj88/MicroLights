import * as React from "react";
import { Checkbox } from "@fluentui/react-components";
import { FlexRow } from "./flex/FlexRow";
import { FlexColumn } from "./flex/FlexColumn";

interface FingerCheckboxAreaProps {
  selectedFingers: number[];
  disabledFingers: number[];
  onSelectedFingersChange: (selectedFingers: number[]) => void;
}

export const FingerCheckboxArea: React.FC<FingerCheckboxAreaProps> = ({
  selectedFingers,
  disabledFingers,
  onSelectedFingersChange,
}) => {
  const [leftPinky, setLeftPinky] = React.useState(selectedFingers.includes(0));
  const [leftRing, setLeftRing] = React.useState(selectedFingers.includes(1));
  const [leftMiddle, setLeftMiddle] = React.useState(
    selectedFingers.includes(2),
  );
  const [leftIndex, setLeftIndex] = React.useState(selectedFingers.includes(3));
  const [leftThumb, setLeftThumb] = React.useState(selectedFingers.includes(4));
  const [rightThumb, setRightThumb] = React.useState(
    selectedFingers.includes(5),
  );
  const [rightIndex, setRightIndex] = React.useState(
    selectedFingers.includes(6),
  );
  const [rightMiddle, setRightMiddle] = React.useState(
    selectedFingers.includes(7),
  );
  const [rightRing, setRightRing] = React.useState(selectedFingers.includes(8));
  const [rightPinky, setRightPinky] = React.useState(
    selectedFingers.includes(9),
  );

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

  const handleCheckboxChange = (index: number, checked: boolean) => {
    const newSelectedFingers = checked
      ? [...selectedFingers, index]
      : selectedFingers.filter((finger) => finger !== index);
    onSelectedFingersChange(newSelectedFingers);
  };

  return (
    <>
      <Checkbox
        disabled={disabledFingers.length > 0}
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
          onSelectedFingersChange(
            checked ? [0, 1, 2, 3, 4, 5, 6, 7, 8, 9] : [],
          );
        }}
        label="All options"
      />
      <FlexRow>
        <FlexColumn>
          <Checkbox
            checked={leftPinky}
            disabled={disabledFingers.includes(0)}
            onChange={() => {
              setLeftPinky((checked) => !checked);
              handleCheckboxChange(0, !leftPinky);
            }}
            label="Left Pinky"
          />
          <Checkbox
            checked={leftRing}
            disabled={disabledFingers.includes(1)}
            onChange={() => {
              setLeftRing((checked) => !checked);
              handleCheckboxChange(1, !leftRing);
            }}
            label="Left Ring"
          />
          <Checkbox
            checked={leftMiddle}
            disabled={disabledFingers.includes(2)}
            onChange={() => {
              setLeftMiddle((checked) => !checked);
              handleCheckboxChange(2, !leftMiddle);
            }}
            label="Left Middle"
          />
          <Checkbox
            checked={leftIndex}
            disabled={disabledFingers.includes(3)}
            onChange={() => {
              setLeftIndex((checked) => !checked);
              handleCheckboxChange(3, !leftIndex);
            }}
            label="Left Index"
          />
          <Checkbox
            checked={leftThumb}
            disabled={disabledFingers.includes(4)}
            onChange={() => {
              setLeftThumb((checked) => !checked);
              handleCheckboxChange(4, !leftThumb);
            }}
            label="Left Thumb"
          />
        </FlexColumn>
        <FlexColumn>
          <Checkbox
            checked={rightPinky}
            disabled={disabledFingers.includes(9)}
            onChange={() => {
              setRightPinky((checked) => !checked);
              handleCheckboxChange(9, !rightPinky);
            }}
            label="Right Pinky"
          />
          <Checkbox
            checked={rightRing}
            disabled={disabledFingers.includes(8)}
            onChange={() => {
              setRightRing((checked) => !checked);
              handleCheckboxChange(8, !rightRing);
            }}
            label="Right Ring"
          />
          <Checkbox
            checked={rightMiddle}
            disabled={disabledFingers.includes(7)}
            onChange={() => {
              setRightMiddle((checked) => !checked);
              handleCheckboxChange(7, !rightMiddle);
            }}
            label="Right Middle"
          />
          <Checkbox
            checked={rightIndex}
            disabled={disabledFingers.includes(6)}
            onChange={() => {
              setRightIndex((checked) => !checked);
              handleCheckboxChange(6, !rightIndex);
            }}
            label="Right Index"
          />
          <Checkbox
            checked={rightThumb}
            disabled={disabledFingers.includes(5)}
            onChange={() => {
              setRightThumb((checked) => !checked);
              handleCheckboxChange(5, !rightThumb);
            }}
            label="Right Thumb"
          />
        </FlexColumn>
      </FlexRow>
    </>
  );
};

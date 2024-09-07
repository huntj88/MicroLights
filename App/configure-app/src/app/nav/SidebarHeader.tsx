import styled from "@emotion/styled";
import React from "react";
import { makeStyles } from "@fluentui/react-components";
import { Text } from "@fluentui/react-components";

interface SidebarHeaderProps extends React.HTMLAttributes<HTMLDivElement> {
  children?: React.ReactNode;
  rtl: boolean;
}

const useStyles = makeStyles({
  header: {
    height: "64px",
    minHeight: "64px",
    width: "100%",
    overflow: "hidden",
    display: "flex",
    alignItems: "center",
    padding: "20px",
  },
  text: {
    fontSize: "20px",
    lineHeight: "30px",
    fontWeight: 700,
    color: "#0098e5",
    margin: 0,
    whiteSpace: "nowrap",
  },
});

// TODO: migrate to makeStyles
const StyledLogo = styled.div<{ rtl?: boolean }>`
  width: 64px;
  min-width: 64px;
  height: 35px;
  min-height: 35px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 8px;
  color: white;
  font-size: 24px;
  font-weight: 700;
  background-color: #009fdb;
  background: linear-gradient(45deg, rgb(21 87 205) 0%, rgb(90 225 255) 100%);
  margin-right: 20px;
`;

export const SidebarHeader: React.FC<SidebarHeaderProps> = ({
  children,
  rtl,
  ...rest
}) => {
  const styles = useStyles();

  return (
    <div className={styles.header}>
      <div style={{ display: "flex", alignItems: "center" }}>
        <StyledLogo rtl={rtl}>FAF</StyledLogo>
        <Text className={styles.text}>Flow Art Forge</Text>
      </div>
    </div>
  );
};

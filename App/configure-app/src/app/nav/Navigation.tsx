"use client";

import React, { ReactNode } from "react";
import {
  Menu,
  menuClasses,
  MenuItem,
  MenuItemStyles,
  Sidebar,
  SubMenu,
} from "react-pro-sidebar";
import { SidebarHeader } from "./SidebarHeader";
import {
  Delete48Regular,
  Desk48Regular,
  Diamond48Regular,
  Doctor48Regular,
  Glasses48Regular,
  Guest48Regular,
  PanelLeftExpandRegular,
  PanelLeftContractRegular,
} from "@fluentui/react-icons";
import {
  makeStyles,
  Text,
  webDarkTheme,
  webLightTheme,
} from "@fluentui/react-components";
import { useThemeContext } from "../ThemeProvider";
import { useRouter } from "next/navigation";

const themes = {
  light: {
    sidebar: {
      backgroundColor: webLightTheme.colorBrandBackground2,
    },
    menu: {
      menuContent: webLightTheme.colorBrandBackground2Hover,
      icon: webLightTheme.colorBrandForeground2,
      hover: {
        backgroundColor: webLightTheme.colorBrandBackground2Hover,
      },
      disabled: {
        color: webLightTheme.colorNeutralForegroundDisabled,
      },
    },
  },
  dark: {
    sidebar: {
      backgroundColor: webDarkTheme.colorBrandBackground2,
    },
    menu: {
      menuContent: webDarkTheme.colorBrandBackground2Hover,
      icon: webDarkTheme.colorBrandForeground2,
      hover: {
        backgroundColor: webDarkTheme.colorBrandBackground2Hover,
      },
      disabled: {
        color: webDarkTheme.colorNeutralForegroundDisabled,
      },
    },
  },
};

const useStyles = makeStyles({
  extraText: {
    fontSize: "12px",
    lineHeight: "18px",
    fontWeight: 600,
    margin: 0,
    whiteSpace: "nowrap",
    letterSpacing: "0.5px",
  },
});

// hex to rgba converter
const hexToRgba = (hex: string, alpha: number) => {
  const r = parseInt(hex.slice(1, 3), 16);
  const g = parseInt(hex.slice(3, 5), 16);
  const b = parseInt(hex.slice(5, 7), 16);

  return `rgba(${r}, ${g}, ${b}, ${alpha})`;
};

export const Navigation: React.FC<{ children: ReactNode }> = ({ children }) => {
  const [collapsed, setCollapsed] = React.useState(false);
  const [toggled, setToggled] = React.useState(false);
  const [broken, setBroken] = React.useState(false);
  const styles = useStyles();
  const { theme, setTheme } = useThemeContext();
  const router = useRouter();

  const menuItemStyles: MenuItemStyles = {
    root: {
      fontSize: "13px",
      fontWeight: 400,
    },
    icon: {
      color: themes[theme].menu.icon,
      [`&.${menuClasses.disabled}`]: {
        color: themes[theme].menu.disabled.color,
      },
    },
    SubMenuExpandIcon: {
      color: "#b6b7b9",
    },
    subMenuContent: ({ level }) => ({
      backgroundColor:
        level === 0
          ? hexToRgba(themes[theme].menu.menuContent, !collapsed ? 0.4 : 1)
          : "transparent",
    }),
    button: {
      [`&.${menuClasses.disabled}`]: {
        color: themes[theme].menu.disabled.color,
      },
      "&:hover": {
        backgroundColor: hexToRgba(themes[theme].menu.hover.backgroundColor, 1),
      },
    },
    label: ({ open }) => ({
      fontWeight: open ? 600 : undefined,
    }),
  };

  return (
    <div
      style={{
        display: "flex",
        height: "100%",
      }}
    >
      <Sidebar
        collapsed={collapsed}
        toggled={toggled}
        onBackdropClick={() => setToggled(false)}
        onBreakPoint={setBroken}
        rtl={false}
        breakPoint="md"
        backgroundColor={hexToRgba(themes[theme].sidebar.backgroundColor, 1)}
        width="300px"
        collapsedWidth="100px"
      >
        <div
          style={{ display: "flex", flexDirection: "column", height: "100%" }}
        >
          <SidebarHeader style={{ marginBottom: "24px", marginTop: "16px" }} />
          <div style={{ flex: 1, marginBottom: "32px", paddingLeft: "10px" }}>
            <Menu menuItemStyles={menuItemStyles}>
              <SubMenu
                label={collapsed ? "" : "Create"}
                icon={<Doctor48Regular />}
              >
                <MenuItem
                  onClick={() => {
                    router.replace("/create");
                  }}
                >
                  Bulb
                </MenuItem>
                <MenuItem>RGB</MenuItem>
              </SubMenu>
              <MenuItem
                onClick={() => {
                  router.replace("/browse");
                }}
                icon={<Diamond48Regular />}
              >
                {collapsed ? "" : "Browse"}
              </MenuItem>

              <MenuItem icon={<Desk48Regular />}>
                {collapsed ? "" : "Program"}
              </MenuItem>
            </Menu>

            <div
              style={{
                padding: "0 24px",
                marginBottom: "8px",
                marginTop: "32px",
              }}
            >
              <Text
                className={styles.extraText}
                style={{ opacity: collapsed ? 0 : 0.7 }}
              >
                Extra
              </Text>
            </div>

            <Menu menuItemStyles={menuItemStyles}>
              <SubMenu
                label={collapsed ? "" : "Theme"}
                icon={<Delete48Regular />}
              >
                <MenuItem onClick={() => setTheme("dark")}>Dark</MenuItem>
                <MenuItem onClick={() => setTheme("light")}>Light</MenuItem>
              </SubMenu>
              <MenuItem icon={<Glasses48Regular />}>
                {collapsed ? "" : "Documentation"}
              </MenuItem>
              <MenuItem disabled icon={<Guest48Regular />}>
                {collapsed ? "" : "Examples"}
              </MenuItem>
              <MenuItem
                onClick={() => setCollapsed((prev) => !prev)}
                icon={
                  collapsed ? (
                    <PanelLeftExpandRegular
                      style={{ width: "48px", height: "48px" }}
                    />
                  ) : (
                    <PanelLeftContractRegular
                      style={{ width: "48px", height: "48px" }}
                    />
                  )
                }
              >
                {collapsed ? "" : "Collapse"}
              </MenuItem>
            </Menu>
          </div>
        </div>
      </Sidebar>

      <main>
        <div style={{ padding: "16px 24px", color: "#44596e" }}>
          <div style={{ marginBottom: "16px" }}>
            {broken && (
              <button
                className="sb-button"
                onClick={() => setToggled(!toggled)}
              >
                Toggle
              </button>
            )}
            {children}
          </div>
        </div>
      </main>
    </div>
  );
};

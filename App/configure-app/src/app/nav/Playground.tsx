import React from "react";
import {
  Menu,
  menuClasses,
  MenuItem,
  MenuItemStyles,
  Sidebar,
  SubMenu,
} from "react-pro-sidebar";
import { SidebarHeader } from "./SidebarHeader";
import { Typography } from "./typography";
import {
  Delete48Regular,
  Desk48Regular,
  Diamond48Regular,
  Doctor48Regular,
  Food48Filled,
  Games48Regular,
  Glasses48Regular,
  Guest48Regular,
} from "@fluentui/react-icons";
import { Switch } from "./Switch";
import { Divider } from "@fluentui/react-components";

// TODO: integrate with fluentUI themeing instead of this
type Theme = "light" | "dark";

const themes = {
  light: {
    sidebar: {
      backgroundColor: "#ffffff",
      color: "#607489",
    },
    menu: {
      menuContent: "#fbfcfd",
      icon: "#0098e5",
      hover: {
        backgroundColor: "#c5e4ff",
        color: "#44596e",
      },
      disabled: {
        color: "#9fb6cf",
      },
    },
  },
  dark: {
    sidebar: {
      backgroundColor: "#0b2948",
      color: "#8ba1b7",
    },
    menu: {
      menuContent: "#082440",
      icon: "#59d0ff",
      hover: {
        backgroundColor: "#00458b",
        color: "#b6c8d9",
      },
      disabled: {
        color: "#3e5e7e",
      },
    },
  },
};

// hex to rgba converter
const hexToRgba = (hex: string, alpha: number) => {
  const r = parseInt(hex.slice(1, 3), 16);
  const g = parseInt(hex.slice(3, 5), 16);
  const b = parseInt(hex.slice(5, 7), 16);

  return `rgba(${r}, ${g}, ${b}, ${alpha})`;
};

export const Playground: React.FC = () => {
  const [collapsed, setCollapsed] = React.useState(false);
  const [toggled, setToggled] = React.useState(false);
  const [broken, setBroken] = React.useState(false);
  const [theme, setTheme] = React.useState<Theme>("dark");

  // handle on theme change event
  const handleThemeChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setTheme(e.target.checked ? "dark" : "light");
  };

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
        color: themes[theme].menu.hover.color,
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
        image="https://user-images.githubusercontent.com/25878302/144499035-2911184c-76d3-4611-86e7-bc4e8ff84ff5.jpg"
        rtl={false}
        breakPoint="md"
        backgroundColor={hexToRgba(themes[theme].sidebar.backgroundColor, 1)}
        width="300px"
        collapsedWidth="100px"
        rootStyles={{
          color: themes[theme].sidebar.color,
        }}
      >
        <div
          style={{ display: "flex", flexDirection: "column", height: "100%" }}
        >
          <SidebarHeader
            rtl={false}
            style={{ marginBottom: "24px", marginTop: "16px" }}
          />
          <div style={{ flex: 1, marginBottom: "32px", paddingLeft: "10px" }}>
            <Menu menuItemStyles={menuItemStyles}>
              <SubMenu
                label={collapsed ? "" : "Create"}
                icon={<Doctor48Regular />}
              >
                <MenuItem>Bulb</MenuItem>
                <MenuItem>RGB</MenuItem>
              </SubMenu>
              <MenuItem icon={<Diamond48Regular />}>
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
              <Typography
                variant="body2"
                fontWeight={600}
                style={{ opacity: collapsed ? 0 : 0.7, letterSpacing: "0.5px" }}
              >
                Extra
              </Typography>
            </div>

            <Menu menuItemStyles={menuItemStyles}>
            <SubMenu
                label={collapsed ? "" : "Theme"}
                icon={<Delete48Regular />}
              >
                <MenuItem> Dark</MenuItem>
                <MenuItem> Light</MenuItem>
              </SubMenu>
              <MenuItem icon={<Glasses48Regular />}>
                {collapsed ? "" : "Documentation"}
              </MenuItem>
              <MenuItem disabled icon={<Guest48Regular />}>
                {collapsed ? "" : "Examples"}
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
          </div>
          <div style={{ marginBottom: "48px" }}>
            <Typography variant="h4" fontWeight={600}>
              React Pro Sidebar
            </Typography>
            <Typography variant="body2">
              React Pro Sidebar provides a set of components for creating high
              level and customizable side navigation
            </Typography>
          </div>

          <div style={{ padding: "0 8px" }}>
            <div style={{ marginBottom: 16 }}>
              <Switch
                id="collapse"
                checked={collapsed}
                onChange={() => setCollapsed(!collapsed)}
                label="Collapse"
              />
            </div>

            <div style={{ marginBottom: 16 }}>
              <Switch
                id="theme"
                checked={theme === "dark"}
                onChange={handleThemeChange}
                label="Dark theme"
              />
            </div>
          </div>
        </div>
      </main>
    </div>
  );
};

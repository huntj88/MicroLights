"use client";

import * as React from "react";
import {
  FluentProvider,
  SSRProvider,
  RendererProvider,
  createDOMRenderer,
  renderToStyleElements,
  webLightTheme,
  webDarkTheme,
} from "@fluentui/react-components";
import { useServerInsertedHTML } from "next/navigation";
import { ThemeProvider, useThemeContext } from "./ThemeProvider";

export function Providers({ children }: { children: React.ReactNode }) {
  const [renderer] = React.useState(() => createDOMRenderer());
  const didRenderRef = React.useRef(false);

  useServerInsertedHTML(() => {
    if (didRenderRef.current) {
      return;
    }
    didRenderRef.current = true;
    return <>{renderToStyleElements(renderer)}</>;
  });

  return (
    <ThemeProvider>
      <RendererProvider renderer={renderer}>
        <SSRProvider>
          <WrappedFluentProvider>{children}</WrappedFluentProvider>
        </SSRProvider>
      </RendererProvider>
    </ThemeProvider>
  );
}

const WrappedFluentProvider = ({ children }: { children: React.ReactNode }) => {
  const { theme } = useThemeContext();
  // Set the app's theme to a corresponding Fluent UI theme.
  const currentTheme = theme === "light" ? webLightTheme : webDarkTheme;

  return <FluentProvider theme={currentTheme}>{children}</FluentProvider>;
};

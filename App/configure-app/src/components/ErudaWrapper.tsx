"use client";

import { useEffect } from "react";

export default function ErudaWrapper() {
  useEffect(() => {
    // use effect and dynamic import used to prevent ssr
    import("eruda").then((eruda) => eruda.default.init());
  }, []);
  return <></>;
}

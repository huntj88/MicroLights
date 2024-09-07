"use client";

import { useEffect } from "react";

export default function ErudaWrapper() {
  useEffect(() => {
    import("eruda").then((eruda) => eruda.default.init());
  }, []);
  return <></>;
}

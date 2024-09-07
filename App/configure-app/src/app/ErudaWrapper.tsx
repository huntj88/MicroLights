"use client";

import eruda from "eruda";
import { useCallback, useEffect } from "react";

export default function ErudaWrapper() {
  useEffect(() => {
    eruda.init();
  }, []);

  // TODO: post messaging should be setup in bluetooth manager/provider?
  const onMessage = useCallback((message: any) => {
    console.log("onMessage", message);
  }, []);

  return (
    <></>
  );
}
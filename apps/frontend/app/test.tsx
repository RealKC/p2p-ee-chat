import { useEffect, useState } from "react";
import { add } from "./wasm.client";

export function Aaa() {
  const [state, setState] = useState(0);

  async function onClick() {
    setState((prev) => {
      return add(prev, 1);
    });
  }

  return (
    <button type="button" onClick={onClick} className="text-red-500">
      {state}
    </button>
  );
}

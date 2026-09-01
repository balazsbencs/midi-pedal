import { useState } from "react";

import type { DeviceSession } from "../../device/DeviceSession";
import type { ConfigDocumentV1 } from "@midi-pedal/protocol";

interface ConnectionControlProps {
  session: DeviceSession;
  onConnected: (document: ConfigDocumentV1, metadata: { imageCrc32: number; imageSize: number; sequence: number; activeSlot: "A" | "B" }) => void;
  onError: (message: string) => void;
}

export function ConnectionControl({ session, onConnected, onError }: ConnectionControlProps) {
  const [busy, setBusy] = useState(false);
  const connect = async () => {
    setBusy(true);
    try {
      const { info } = await session.connect();
      const document = await session.readConfiguration();
      onConnected(document, info);
    } catch (error) {
      onError(error instanceof Error ? error.message : "Could not connect to the pedal");
    } finally { setBusy(false); }
  };
  return <button type="button" className="connect-button" disabled={busy} onClick={() => void connect()}>{busy ? "Connecting…" : "Connect pedal"}</button>;
}


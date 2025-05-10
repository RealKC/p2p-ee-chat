import type { MetaFunction } from "@remix-run/node";
import YourId from "~/components/your-id";
import ConnectToUser from "~/connect-to-user.client";
import { P2PProvider, useP2P } from "~/p2p";

export const meta: MetaFunction = () => {
  return [
    { title: "New Remix App" },
    { name: "description", content: "Welcome to Remix!" },
  ];
};

export default function Index() {
  return (
    <div className="flex flex-col h-screen items-center justify-center">
      <P2PProvider>
        <YourId />
        <ConnectToUser />
      </P2PProvider>
    </div>
  );
}

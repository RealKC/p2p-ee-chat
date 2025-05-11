import { Link } from "@remix-run/react";
import { HOME } from "~/chats";
import { useP2P } from "~/p2p.client";
import ChatLink from "./chat-link";

export default function ChatsSidebar() {
  const { connections } = useP2P();

  return (
    <div className="col-auto border-cyan-400 border-solid border-2 p-2 m-">
      <ChatLink chat={HOME}>Main Screen</ChatLink>
      <br />
      Chats you're part of:
      <br />
      {[...connections.map.keys()].map((key) => (
        <ChatLink chat={key} key={key}>
          {key}
        </ChatLink>
      ))}
    </div>
  );
}

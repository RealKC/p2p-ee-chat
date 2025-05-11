import { useParams } from "@remix-run/react";
import { useP2P } from "~/p2p";

export default function Chat() {
  const params = useParams();
  const { chats } = useP2P();
}

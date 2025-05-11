import type { PropsWithChildren } from "react";
import { useChats } from "~/chats";

export interface ChatLinkProps {
  chat: string;
}

export default function ChatLink({ chat }: ChatLinkProps & PropsWithChildren) {
  const {
    actions: { setCurrentChat },
  } = useChats();

  function onClick() {
    setCurrentChat(chat);
  }

  return (
    <button onClick={onClick} type="button">
      {chat}
    </button>
  );
}

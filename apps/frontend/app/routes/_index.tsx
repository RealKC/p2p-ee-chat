import type { MetaFunction } from "@remix-run/node";
import { Suspense } from "react";
import { ChatsProvider, HOME, useChats } from "~/chats";
import ChatView from "~/components/chat-view";
import ChatsSidebar from "~/components/chats-sidebar";
import YourId from "~/components/your-id";
import ConnectToUser from "~/connect-to-user.client";
import { P2PProvider, useP2P } from "~/p2p.client";

export const meta: MetaFunction = () => {
  return [
    { title: "CSD Chat App" },
    { name: "description", content: "E2E P2P chat app" },
  ];
};

function MainBody() {
  const {
    state: { currentChat },
  } = useChats();

  return (
    <div>
      {currentChat === HOME ? (
        <div>
          <YourId />
          <ConnectToUser />
        </div>
      ) : (
        <ChatView peerId={currentChat} />
      )}
    </div>
  );
}

export default function Index() {
  return (
    <div className="flex flex-col h-screen items-center justify-center">
      <Suspense>
        <P2PProvider>
          <ChatsProvider>
            <div className="grid grid-cols-2 space-x-8">
              <ChatsSidebar />
              <MainBody />
            </div>
          </ChatsProvider>
        </P2PProvider>
      </Suspense>
    </div>
  );
}

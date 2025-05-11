import React, { type PropsWithChildren, useContext, useState } from "react";

type ChatData = {
  state: {
    currentChat: string;
  };
  actions: {
    setCurrentChat: React.Dispatch<React.SetStateAction<string>>;
  };
};

const ChatsCtx = React.createContext<ChatData | null>(null);

export const HOME = "~~";

export function useChats() {
  const chat = useContext(ChatsCtx);

  return chat!;
}

export function ChatsProvider(props: PropsWithChildren) {
  const [currentChat, setCurrentChat] = useState(HOME);

  return (
    <ChatsCtx.Provider
      value={{
        state: { currentChat },
        actions: { setCurrentChat },
      }}
    >
      {props.children}
    </ChatsCtx.Provider>
  );
}

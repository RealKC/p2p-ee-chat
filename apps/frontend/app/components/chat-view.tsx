import { type SubmitHandler, useForm } from "react-hook-form";
import { useChats } from "~/chats";
import { useP2P } from "~/p2p";

type Inputs = {
  message: string;
};

interface ChatViewProps {
  peerId: string;
}

export default function ChatView({ peerId }: ChatViewProps) {
  const {
    state: { currentChat },
  } = useChats();

  const { chats, sendChatMessage } = useP2P();

  const { handleSubmit, register } = useForm<Inputs>();

  const onSubmit: SubmitHandler<Inputs> = (data) => {
    sendChatMessage(peerId, data.message);
  };

  return (
    <div>
      {chats.map.get(currentChat)?.map((val, index) => (
        // biome-ignore lint/suspicious/noArrayIndexKey: <explanation>
        <div key={index}>
          {val.sender} said: <br /> {val.message}{" "}
        </div>
      ))}

      <form className="flex flex-row" onSubmit={handleSubmit(onSubmit)}>
        <input
          placeholder="your message..."
          autoComplete="off"
          {...register("message")}
        />

        <input type="submit" />
      </form>
    </div>
  );
}

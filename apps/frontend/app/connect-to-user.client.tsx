import { type SubmitHandler, useForm } from "react-hook-form";
import { useP2P } from "./p2p";

type Inputs = {
  chatId: string;
  username: string;
};

export default function ConnectToUser() {
  const { connectToPeer, setUsername } = useP2P();

  const { handleSubmit, register } = useForm<Inputs>();

  const onSubmit: SubmitHandler<Inputs> = (data) => {
    setUsername(data.username);
    connectToPeer(data.chatId);
  };

  return (
    <form className="flex flex-col" onSubmit={handleSubmit(onSubmit)}>
      <input
        defaultValue="user"
        placeholder="username"
        {...register("username")}
      />
      <input placeholder="chat id" autoComplete="off" {...register("chatId")} />

      <input type="submit" />
    </form>
  );
}

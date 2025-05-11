import { useP2P } from "~/p2p.client";

export default function YourId() {
  const { userId } = useP2P();

  return <span>Your id is: {userId}</span>;
}

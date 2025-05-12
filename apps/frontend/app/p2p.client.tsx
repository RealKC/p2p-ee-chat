import { CryptoU8Vec, DHKey, EncryptionKey } from "@csd/crypto";
import { Await, useAsyncValue } from "@remix-run/react";
import { Mutex } from "async-mutex";
import React, {
  type PropsWithChildren,
  Suspense,
  useContext,
  useEffect,
} from "react";
import { useState } from "react";
import useWebSocket, { type SendMessage } from "react-use-websocket";
import { z } from "zod";
import { bytesToBase64DataUrl, dataUrlToBytes } from "~/base64";
import { type crypto, decryptMessage, encryptMessage } from "./crypto.client";

const ClientMessageTypes = ["offer", "answer", "ice-candidate"] as const;

type ClientMessageType = (typeof ClientMessageTypes)[number];

const SignalMessageTypes = ["identify", ...ClientMessageTypes] as const;

const SignalMessage = z.object({
  data: z.string().nullish(),
  type: z.enum(SignalMessageTypes),
  userId: z.string(),
});

type SetState<S> = React.Dispatch<React.SetStateAction<S>>;

type ConnectionsMap = Map<string, RTCPeerConnection>;

type P2PChannel = {
  channel: RTCDataChannel;
  state: "negotiating" | "negotiated";
  key?: crypto.EncryptionKey;
};
type DataChannels = Map<string, P2PChannel>;

export type ChatMessage = {
  sender: "you" | "they";
  message: string;
};

type P2PContextData = {
  iceServers: RTCIceServer[];
  mutex: Mutex;
  state: {
    userId?: string;
    username?: string;
    connections: { map: ConnectionsMap };
    dataChannels: { map: DataChannels };
    chats: { map: Map<string, ChatMessage[]> };
    inProgressMessages: { map: Map<string, Map<number, Uint8Array>> };
  };
  actions: {
    sendToSignalingServer: (
      sdp: string,
      peerId: string,
      type: ClientMessageType,
    ) => Promise<void>;
    createConnection: (peerId: string) => RTCPeerConnection;
    setUserId: SetState<string | undefined>;
    setUsername: SetState<string | undefined>;
    setConnections: SetState<{ map: ConnectionsMap }>;
    setDataChannels: SetState<{ map: DataChannels }>;
    sendWSMessage: SendMessage;
    setChats: SetState<{ map: Map<string, ChatMessage[]> }>;
    setInProgressMessages: SetState<{
      map: Map<string, Map<number, Uint8Array>>;
    }>;
  };
};

const P2PContext = React.createContext<P2PContextData | null>(null);

function assignDataChannelEventHandlers(
  dataChannel: RTCDataChannel,
  peerId: string,
  chats: { map: Map<string, ChatMessage[]> },
  dataChannels: { map: DataChannels },
  setChats: SetState<{ map: Map<string, ChatMessage[]> }>,
  secret: crypto.DHKey,
  inProgressMessages: { map: Map<string, Map<number, Uint8Array>> },
  mutex: Mutex,
) {
  const inProgress = new Map<number, Uint8Array>();

  type ChunkedMessage = {
    id: number;
    end: boolean;
    chunk?: string;
  };

  const handleOnMessage = async (event: MessageEvent) => {
    console.log(`data channel message ${event.data}`);

    const channel = dataChannels.map.get(peerId);
    if (!channel) {
      console.log(`received message for ${peerId} but the channel is dead`);
      return;
    }

    if (channel.state === "negotiating") {
      const message = new Uint8Array(event.data);
      const vec = new CryptoU8Vec();
      for (const byte of message) {
        vec.push_back(byte);
      }
      const theirs = DHKey.fromBytes(vec);

      channel.key = EncryptionKey.fromDiffieHellman(secret, theirs);
      channel.state = "negotiated";
      console.log("finsihed negotiating key");
      return;
    }

    const desered: ChunkedMessage = JSON.parse(event.data);

    console.log(`received ${desered.id}`);

    if (!desered.end) {
      const newChunk = await dataUrlToBytes(desered.chunk!)!;
      const inProgressForPeer = inProgressMessages.map.get(peerId);
      const currentChunk = inProgressForPeer?.get(desered.id);

      const newArray = new Uint8Array(
        newChunk.length + (currentChunk?.length ?? 0),
      );

      if (currentChunk) {
        newArray.set(currentChunk, 0);
      }
      newArray.set(newChunk, currentChunk?.length ?? 0);

      if (!inProgressForPeer) {
        const forPeer = new Map<number, Uint8Array>();
        forPeer.set(desered.id, newArray);
        inProgressMessages.map.set(peerId, forPeer);
      } else {
        inProgressForPeer.set(desered.id, newArray);
        inProgressMessages.map.set(peerId, inProgressForPeer);
        console.log(`set ${peerId} with ${desered.id}`);
      }

      console.log("not end: ", inProgressMessages);

      return;
    }

    console.log("end: ", inProgressMessages);
    const message = inProgressMessages.map.get(peerId)!.get(desered.id)!;

    const key = channel.key!;

    console.log(typeof message);
    const decrypted = decryptMessage(key, message);

    console.log(`decrypted result is "${decrypted}"`);

    const log = chats.map.get(peerId);
    if (log) {
      log.push({ message: decrypted, sender: "they" });
    } else {
      chats.map.set(peerId, [{ message: decrypted, sender: "they" }]);
    }

    setChats({ map: chats.map });
  };

  dataChannel.onmessage = async (event) => {
    await mutex.runExclusive(async () => await handleOnMessage(event));
  };

  dataChannel.onopen = (event) => {
    console.log("send my key");

    const peer = dataChannels.map.get(peerId);

    return dataChannel.send(secret.computePublic().bytes());
  };
  dataChannel.onclose = () => console.log("data channel closed");
}

export function useP2P() {
  const {
    state: {
      userId,
      username,
      connections,
      dataChannels,
      chats,
      inProgressMessages,
    },
    actions: {
      sendToSignalingServer,
      createConnection,
      setUsername,
      setDataChannels,
      setChats,
    },
    mutex,
  } = useContext(P2PContext)!;

  async function connectToPeer(peerId: string) {
    try {
      const connection = createConnection(peerId);

      const dataChannel = connection.createDataChannel(
        `${username}-${peerId}`,
        {
          ordered: true,
        },
      );

      setDataChannels((prev) => ({
        map: prev.map.set(peerId, {
          channel: dataChannel!,
          state: "negotiating",
        }),
      }));

      assignDataChannelEventHandlers(
        dataChannel,
        peerId,
        chats,
        dataChannels,
        setChats,
        DHKey.makePrivate(),
        inProgressMessages,
        mutex,
      );

      const offer = await connection.createOffer();
      connection.setLocalDescription(offer);
      console.log("send offer");
      sendToSignalingServer(offer.sdp!, peerId, "offer");
    } catch (e) {
      console.error(`Got an error trying to connect to client: ${e}`);
    }
  }

  async function sendChatMessage(peerId: string, message: string) {
    const connection = connections.map.get(peerId);
    if (!connection) {
      return;
    }

    const channel = dataChannels.map.get(peerId);

    if (!channel) {
      console.error(`No connection for ${peerId}`);
      return;
    }

    const log = chats.map.get(peerId);
    if (log) {
      log.push({ message, sender: "you" });
    } else {
      chats.map.set(peerId, [{ message, sender: "you" }]);
    }

    setChats({ map: chats.map });

    const encrypted = encryptMessage(channel.key!, message);

    let index = 0;
    const id = Date.now();

    while (index < encrypted.length) {
      const chunk = await bytesToBase64DataUrl(
        new Uint8Array(encrypted, index, 256),
      );
      console.log("chunk:", chunk);
      channel.channel.send(JSON.stringify({ id, chunk, end: false }));
      index += 256;
    }
    channel.channel.send(JSON.stringify({ id, end: true }));
  }

  return {
    connectToPeer,
    username,
    setUsername,
    userId,
    sendChatMessage,
    connections,
    chats,
  };
}

export function P2PProvider(props: PropsWithChildren) {
  const signalingServerAddress = "192.168.11.94:8080";

  const fetchIceServers = async () => {
    const response = await fetch(`http://${signalingServerAddress}/ice`, {
      method: "GET",
    });

    return await response.json();
  };

  function InnerElemens() {
    const iceServers = useAsyncValue() as RTCIceServer[];

    const { sendMessage, lastMessage } = useWebSocket(
      `ws://${signalingServerAddress}/ws`,
    );

    const [userId, setUserId] = useState<string | undefined>();

    const [username, setUsername] = useState<string | undefined>();

    const [connections, setConnections] = useState({
      map: new Map<string, RTCPeerConnection>(),
    });

    const [dataChannels, setDataChannels] = useState({
      map: new Map<string, P2PChannel>(),
    });

    const [chats, setChats] = useState({
      map: new Map<string, ChatMessage[]>(),
    });

    const [inProgressMessages, setInProgressMessages] = useState({
      map: new Map<string, Map<number, Uint8Array>>(),
    });

    const mutex = new Mutex();

    const createConnection = (peerId: string) => {
      const connection = new RTCPeerConnection({ iceServers });
      connection.onicecandidate = (e) => {
        if (!e.candidate) {
          return;
        }

        sendToSignalingServer(
          JSON.stringify(e.candidate),
          peerId,
          "ice-candidate",
        );
      };
      connection.onsignalingstatechange = (e) =>
        console.log(
          `signalign state: ${connection.signalingState} ${connection.localDescription} ${connection.remoteDescription}`,
        );
      connection.onconnectionstatechange = (e) =>
        console.log(`connection state: ${connection.connectionState}`);

      setConnections((prev) => ({
        map: prev.map.set(peerId, connection),
      }));

      return connection;
    };

    async function sendToSignalingServer(
      sdp: string,
      peerId: string,
      type: ClientMessageType,
    ) {
      sendMessage(
        JSON.stringify({
          data: sdp,
          type,
          userId: peerId,
        }),
      );
    }

    // biome-ignore lint/correctness/useExhaustiveDependencies: <explanation>
    useEffect(() => {
      if (lastMessage === null) {
        return;
      }

      const message = SignalMessage.parse(JSON.parse(lastMessage!.data));
      // console.log(`received ${JSON.stringify(message)}`);

      // console.log(`last message type: ${message.type}`);
      if (message.type === "identify") {
        setUserId(message.userId);
      }

      if (message.type === "offer") {
        console.log("received offer");

        const handleOffer = async () => {
          const connection = createConnection(message.userId);

          connection.ondatachannel = (event) => {
            console.log("new data channel");
            console.log(event);
            const dataChannel = event.channel;
            setDataChannels((prev) => ({
              map: prev.map.set(message.userId, {
                channel: dataChannel,
                state: "negotiating",
              }),
            }));

            assignDataChannelEventHandlers(
              dataChannel,
              message.userId,
              chats,
              dataChannels,
              setChats,
              DHKey.makePrivate(),
              inProgressMessages,
              mutex,
            );
          };

          console.log("have remote offer");

          try {
            await connection.setRemoteDescription({
              sdp: message.data!,
              type: "offer",
            });
            const answer = await connection.createAnswer();
            connection.setLocalDescription(answer);

            sendToSignalingServer(answer.sdp ?? "", message.userId, "answer");
          } catch (e) {
            console.error(`failed to set remote description: ${e}`);
          }
        };

        handleOffer().catch((e) => console.log(`fu kfff ${e}`));
      }

      if (message.type === "answer") {
        console.log("received answer");

        const handleAnswer = async () => {
          const connection = connections?.map.get(message.userId);
          if (!connection) {
            return;
          }

          if (connection.signalingState === "stable") {
            return;
          }

          try {
            await connections?.map.get(message.userId)?.setRemoteDescription({
              sdp: message.data!,
              type: "answer",
            });
          } catch (e) {
            console.error(`failed in answer: ${e}`);
          }
        };

        handleAnswer();
      }

      if (message.type === "ice-candidate") {
        connections?.map
          .get(message.userId)
          ?.addIceCandidate(JSON.parse(message.data!));
      }
    }, [lastMessage]);

    const p2p = {
      iceServers,
      mutex,
      state: {
        userId,
        username,
        connections,
        dataChannels,
        chats,
        inProgressMessages,
      },
      actions: {
        sendWSMessage: sendMessage,
        setUserId,
        setUsername,
        setConnections,
        setDataChannels,
        sendToSignalingServer,
        createConnection,
        setChats,
        setInProgressMessages,
      },
    };

    return (
      <P2PContext.Provider value={p2p}>{props.children}</P2PContext.Provider>
    );
  }

  return (
    <Suspense fallback={<div>Loading...</div>}>
      <Await resolve={fetchIceServers()}>
        <InnerElemens />
      </Await>
    </Suspense>
  );
}

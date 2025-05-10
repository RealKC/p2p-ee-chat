import { Await, useAsyncValue } from "@remix-run/react";
import React, {
  type PropsWithChildren,
  Suspense,
  useContext,
  useEffect,
} from "react";
import { useState } from "react";
import useWebSocket, { type SendMessage } from "react-use-websocket";
import { z } from "zod";

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
type DataChannels = Map<string, RTCDataChannel>;

type P2PContextData = {
  iceServers: RTCIceServer[];
  state: {
    userId?: string;
    username?: string;
    connections: { map: ConnectionsMap };
    dataChannels: { map: DataChannels };
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
  };
};

const P2PContext = React.createContext<P2PContextData | null>(null);

export function useP2P() {
  const {
    state: { userId, username, connections, dataChannels },
    actions: {
      sendToSignalingServer,
      createConnection,
      setUsername,
      setDataChannels,
    },
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
      function onDataChannelStatusChange(_event: Event) {
        console.log(`status change: ${dataChannel!.readyState}`);
      }
      dataChannel.onopen = onDataChannelStatusChange;
      dataChannel.onclose = onDataChannelStatusChange;
      setDataChannels((prev) => ({
        map: prev.map.set(peerId, dataChannel!),
      }));

      const offer = await connection.createOffer();
      connection.setLocalDescription(offer);
      console.log("send offer");
      sendToSignalingServer(offer.sdp!, peerId, "offer");
    } catch (e) {
      console.error(`Got an error trying to connect to client: ${e}`);
    }
  }

  function sendChatMessage(peerId: string, message: string) {
    const connection = connections.map.get(peerId);
    if (!connection) {
      return;
    }

    const dataChannel = dataChannels.map.get(peerId);

    if (!dataChannel) {
      console.error(`No connection for ${peerId}`);
      return;
    }

    console.log(`${dataChannel.readyState}`);

    dataChannel.send(message);
  }

  return {
    connectToPeer,
    username,
    setUsername,
    userId,
    sendChatMessage,
    connections,
  };
}

export function P2PProvider(props: PropsWithChildren) {
  const fetchIceServers = async () => {
    const response = await fetch(
      "https://dumitruac.metered.live/api/v1/turn/credentials?apiKey=7b23492029e22bd7b36e60981b32e39a51e9",
    );

    return await response.json();
  };

  function InnerElemens() {
    const iceServers = useAsyncValue() as RTCIceServer[];

    const { sendMessage, lastMessage } = useWebSocket("ws://localhost:8080/ws");

    const [userId, setUserId] = useState<string | undefined>();

    const [username, setUsername] = useState<string | undefined>();

    const [connections, setConnections] = useState({
      map: new Map<string, RTCPeerConnection>(),
    });

    const [dataChannels, setDataChannels] = useState({
      map: new Map<string, RTCDataChannel>(),
    });

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
              map: prev.map.set(message.userId, dataChannel),
            }));

            dataChannel.onmessage = (event) => {
              console.log(`data channel message ${event.data}`);
            };
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
      state: { userId, username, connections, dataChannels },
      actions: {
        sendWSMessage: sendMessage,
        setUserId,
        setUsername,
        setConnections,
        setDataChannels,
        sendToSignalingServer,
        createConnection,
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

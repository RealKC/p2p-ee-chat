## p2p-ee-chat

This repository contains an end-to-end peer-to-peer encrypted chat app implemented using:

* WebRTC for P2P communication
* Twofish for encryption (implemented in C++ compiled with emscripten to Wasm)
* Diffie-Hellman key exchange (implemented in C++ compiled with emscripten to Wasm)
* Remix and React for the frontend

This application was developed for our Cryptography and Data Security class.

### Application architecture

![system diagram](docs/System%20Diagram.drawio.png)

The app contains two modules:
* a signaling server for the initial phase of estabilishing a WebRTC connection
* a client-side React app that uses browser WebRTC APIs and the aforementioned signaling server to estabilish P2P channels.

The flow when two clients access the browser app and wish to start a chat between them is best explained by the following sequence diagram:

![connection sequence diagram](docs/Connection%20Flow.drawio.png)

import CryptoLib from "./build-wasm/crypto-lib";

export * as crypto from "./build-wasm/crypto-lib";

export const {
  CryptoU8Vec,
  encryptMessage,
  decryptMessage,
  DHKey,
  EncryptionKey,
} = await CryptoLib();

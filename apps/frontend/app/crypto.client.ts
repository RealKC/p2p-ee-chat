import {
  type crypto,
  CryptoU8Vec,
  decryptMessage as decryptMessageRaw,
  encryptMessage as encryptMessageRaw,
} from "@csd/crypto";

export type { crypto } from "@csd/crypto";

export function encryptMessage(
  key: crypto.EncryptionKey,
  message: string,
): Uint8Array {
  const encrypted = encryptMessageRaw(key, message);

  const array = new Array();

  for (let i = 0; i < encrypted.size(); ++i) {
    array.push(encrypted.get(i));
  }

  console.log("encrypted:", new Uint8Array(array));

  return new Uint8Array(array);
}

export function decryptMessage(
  key: crypto.EncryptionKey,
  message: Uint8Array,
): string {
  console.log("decrypting: ", message);

  const encrypted = new CryptoU8Vec();

  for (const byte of message) {
    encrypted.push_back(byte);
  }

  return decryptMessageRaw(key, encrypted);
}

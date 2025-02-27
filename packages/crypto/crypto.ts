import CryptoLib from "./build-wasm/crypto-lib";

const cryptoLib = await CryptoLib();

export function add(a: number, b: number) {
  return cryptoLib.add(a, b);
}

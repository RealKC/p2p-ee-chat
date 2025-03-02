## Development instructions

### Initial setup
1. [Install node](https://nodejs.org/en/download)
2. [Install pnpm](https://pnpm.io/installation)
3. Run `pnpm i` in the root directory of the project

### Adding new features to `packages/crypto`

1. Create a new file
2. Include `common.hpp` (it provides access to commonly needed emscripten and catch2 things, plus useful defines)
3. Create a section that looks like
```cpp
#if IS_WASM
EMSCRIPTEN_BINDINGS(<unique identifier representing your module>)
{
    using namespace emscripten;

    // To export a function to JavaScript
    function("functionName", &function_name);

    // To see more about using the Embind library, refer to its docs here: https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html
}
#endif
```
4. Export the function from the TypeScript `crypto` module like so
```ts
export function functionName(parameters: type...) {
    return cryptoLib.functionName(parameters...);
}
```

### Working on `app/frontend`

* Run `pnpm build` to build it
* Run `pnpm dev` to get a local dev server with hot reload
* Edit the page you want to edit :)

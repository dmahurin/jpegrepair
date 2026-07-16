# jpegrepair

Repair jpeg images, by the following operations.
- Change color components: Y,Cb,Cr
- Insert blocks
- Delete blocks
- Copy relative blocks

## Depends

* libjpeg

## Build

> make

### WebAssembly

The WebAssembly build requires the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html),
CMake, curl, and tar. It downloads and builds a pinned static copy of
libjpeg-turbo, so a WebAssembly libjpeg installation is not otherwise needed.

> make wasm

The output is `dist/wasm/jpegrepair.js` and
`dist/wasm/jpegrepair.wasm`. The generated module works in browsers, Web
Workers, and Node.js. It does not run `main()` automatically; put input files
in Emscripten's virtual filesystem and invoke the existing command-line
interface with `callMain`:

```js
import createJpegRepairModule from "./dist/wasm/jpegrepair.js";

const module = await createJpegRepairModule();
module.FS.writeFile("dark.jpg", inputJpegBytes);
module.callMain(["dark.jpg", "light.jpg", "cdelta", "0", "100"]);
const outputJpegBytes = module.FS.readFile("light.jpg");
```

To use the browser interface, serve the project directory after building:

> python3 -m http.server 8000

Then open `http://localhost:8000/`. The interface accepts a JPEG, builds a
sequence of operations, previews the result, and provides it for download.
Opening `index.html` directly with a `file://` URL will not work because
browsers restrict loading WebAssembly modules that way.

The libjpeg-turbo version and download URL can be overridden when needed:

> make wasm LIBJPEG_TURBO_VERSION=3.1.1 LIBJPEG_TURBO_URL=file:///path/to/libjpeg-turbo.tar.gz

## Usage

> jpegrepair infile outfile OP ...

where OP is: `cdelta` `dest` `insert` `delete` `copy`

Operations are applied from left to right:

- `dest BY BX [BN]` selects the destination MCU at row `BY`, column `BX`.
  With `BN`, it selects that many consecutive MCUs. The destination remains
  active for following operations until another `dest` is encountered.
- `dest BY BX BH BW` selects a rectangular region of `BH` rows by `BW`
  columns.
- `cdelta COMP DC` adds `DC` to the DC coefficient of component `COMP` in
  the selected region. Components are normally `0` for Y (luminance), `1`
  for Cb, and `2` for Cr.
- `insert N` inserts `N` MCUs at the destination, shifting later coefficient
  blocks forward.
- `delete N` deletes `N` MCUs at the destination, shifting later coefficient
  blocks backward.
- `copy DY DX` copies coefficient blocks from the relative MCU offset
  `DY`, `DX` into the selected region.

JPEG DCT blocks are always 8x8 samples, but destination coordinates address
MCUs. An MCU can cover multiple DCT blocks because of chroma subsampling. For
example, a 4:2:2 image uses 16x8-pixel MCUs.

## Examples

Increase luminance.

> jpegrepair dark.jpg light.jpg cdelta 0 100

Fix blueish image.

> jpegrepair blueish.jpg fixed.jpg cdelta 1 -100

Insert 2 blocks at position 50:5

> jpegrepair before.jpg after.jpg dest 50 5 insert 2

Repair `corrupt.jpg`: delete one MCU at row 49, column 48; adjust the Y, Cb,
and Cr DC coefficients in that destination region; then insert two MCUs.

> jpegrepair corrupt.jpg corrupt-repaired.jpg dest 49 48 delete 1 cdelta 0 -30 cdelta 1 35 cdelta 2 -40 insert 2

Copy to position 9:35 2x2 blocks from relative block 1:-20 (1 row forward, 20 columns back).

> jpegrepair before.jpg after.jpg dest 9 35 2 2 copy 1 -20

## License

* jpegrepair.c - See LICENSE
* transupp.c - See README.ijg

# jpegrepair

Repair jpeg images, by the following operations.
- Change color components: Y,Cb,Cr
- Insert blocks
- Delete blocks
- Copy relative blocks

## Build

> gcc jpegrepair.c transupp.c -ljpeg -o jpegrepair

## Examples

Reduce luminance.

> jpegrepair dark.jpg light.jpg cdelta 0 100

Fix blueish image.

> jpegrepair blueish.jpg fixed.jpg cdelta 1 -100

Insert 2 blocks at position 50:5

> jpegrepair before.jpg after.jpg dest 50 5 insert 2

Delete 1 block at position 63:54, and after that, correct luminance.
Delete 1 block at position 112:0

> jpegrepair corrupt.jpg fixed.jpg dest 63 54 delete 1 cdelta 0 -450 dest 112 0 delete 1

## License

jpegrepair.c - See LICENSE

transupp.c - See README.ijg

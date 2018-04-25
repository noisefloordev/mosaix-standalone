This is a standalone implementation of the [Unity Mosaix shader](https://github.com/noisefloordev/mosaix),
with a commandline implementation and a Photoshop filter.  This allows correctly mosaicing transparent images:

![](docs/teapot.png)

Mosaix retains the original shape of the transparent layer, where Photoshop and other mosaic
filters mosaic the shape itself.

The mosaic can also be shifted and applied at an angle (this isn't currently exposed in the
commandline version).


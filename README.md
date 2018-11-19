This is a standalone implementation of the [Unity Mosaix shader](https://github.com/noisefloordev/mosaix),
with a commandline implementation and Photoshop and After Effects plugins.  This allows correctly mosaicing transparent images:

![](docs/teapot.png)

Mosaix retains the original shape of the transparent layer, where Photoshop and other mosaic
filters mosaic the shape itself.

The mosaic can also be shifted and applied at an angle (this isn't currently exposed in the
commandline version).

Photoshop and After Effects installation
----------------------------------------

To install, copy **mosaix-ps.8bf** and **mosaix-afx.aex** to **C:\Program Files\Common Files\Adobe\Plug-Ins\CC**
and restart the application.

In Photoshop, the filter will appear in **Filter > Pixelate > Mosaix**.
In After Effects, the effect will appear in **Effect > Stylize > Mosaix**.

Masking in After Effects
------------------------

![](docs/mask%20example.png)

A mask can be supplied with the After Effects filter to choose which parts of
the image to mosaic.

The MaskOffset property can be used to move the mask around.  For example, this
can be connected to a motion tracker to mosaic the face of a moving person.

This can be better than using compositing masks, because only pixels from within
the mask will be included in the mosaic.

If the distance to the person's face is known, using it to drive the block size
allows the mosaic to scale with distance.  This allows large mosaic blocks to
be used when the person is near the camera, and to scale them down as he gets
further away.  If this is done, the offset should also follow the mask around,
so the mosaic doesn't shift around as the scale changes.

Commandline
-----------

The commandline version supports PNG and EXR files.  Usage:

mosaix.exe [-n] input.exr output.exr block_size

- block_size: The pixel size of the mosaic.

- -n: Don't compress the output file.  This can improve performance for larger images, especially
for EXR output.



#ifndef IMAGE_IO_H
#define IMAGE_IO_H

#include "../mosaix-core/Image.h"

// Loading and saving PNG and EXR files.
namespace ImageHelpers
{
    void ReadImage(Image &image, string filename);
    void ReadPNG(Image &image, string filename);
    void ReadEXR(Image &image, string filename);

    void WriteImage(const Image &image, string filename, bool compression);
    void WritePNG(const Image &image, string filename, bool compression);
    void WriteEXR(const Image &image, string filename, bool compression);
}

#endif

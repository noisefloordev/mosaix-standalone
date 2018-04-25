#ifndef MOSAIC_H
#define MOSAIC_H

#include "Image.h"

#include <vector>
using namespace std;

namespace Mosaic
{
    struct Options
    {
        int block_size = 16;
        float angle = 0;
        int offset_x = 0;
        int offset_y = 0;
        bool operator==(const Options &rhs) const;
    };

    void ApplyMosaic(Image &image, const Options &options);
}

#endif

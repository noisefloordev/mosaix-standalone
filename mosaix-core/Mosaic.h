#ifndef MOSAIC_H
#define MOSAIC_H

#include "Image.h"

#include <vector>
using namespace std;

namespace Mosaic
{
    struct Options
    {
        float block_size = 16;
        float angle = 0;
        int origin_x = 0;
        int origin_y = 0;
        bool operator==(const Options &rhs) const;
    };

    void ApplyMosaic(Image &image, const Options &options);
}

#endif

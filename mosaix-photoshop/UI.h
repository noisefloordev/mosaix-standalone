#ifndef UI_H
#define UI_H

#include "../mosaix-core/Image.h"
#include "../mosaix-core/Mosaic.h"
#include <memory>

bool DoUI(Mosaic::Options &options, std::shared_ptr<const Image> image);

#endif

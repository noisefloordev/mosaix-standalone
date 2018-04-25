#include "Image.h"
#include <algorithm>

void Image::Alloc(int width, int height)
{
    rgba.resize(width*height, Vec4f());
}

Vec4f &Image::ptr(int x, int y)
{
    return rgba[y*width + x];
}

void swap(Image &lhs, Image &rhs)
{
    swap(lhs.width, rhs.width);
    swap(lhs.height, rhs.height);
    swap(lhs.rgba, rhs.rgba);
}


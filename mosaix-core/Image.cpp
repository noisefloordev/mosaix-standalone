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

void Image::TopLeftVisiblePixel(int &out_x, int &out_y) const
{
    out_x = 0;
    out_y = 0;
    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {
            if(ptr(x,y)[3] > 0.01f)
            {
                out_x = x;
                out_y = y;
                return;
            }
        }
    }
}

void Image::BottomRightVisiblePixel(int &out_x, int &out_y) const
{
    out_x = 0;
    out_y = 0;
    for(int y = height-1; y >= 0; --y)
    {
        for(int x = width-1; x >= 0; --x)
        {
            if(ptr(x,y)[3] > 0.01f)
            {
                out_x = x;
                out_y = y;
                return;
            }
        }
    }
}

void Image::CenterVisiblePixel(int &out_x, int &out_y) const
{
    int top_left_x = 0, top_left_y;
    TopLeftVisiblePixel(top_left_x, top_left_y);

    int bottom_right_x = 0, bottom_right_y;
    BottomRightVisiblePixel(bottom_right_x, bottom_right_y);

    out_x = (top_left_x + bottom_right_x) / 2;
    out_y = (top_left_y + bottom_right_y) / 2;
}

void Image::AlphaComposite(shared_ptr<const Image> image)
{
    // Sanity check:
    if(width != image->width || height != image->height)
        return;

    for(int i = 0; i < width*height; ++i)
    {
        Vec4f &bottom = rgba[i];
        const Vec4f &top = image->rgba[i];
        bottom = bottom*(1-top.w) + top;
    }
}


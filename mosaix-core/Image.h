#ifndef Image_h
#define Image_h

#include <vector>
using namespace std;

#include "Vec4f.h"

// A simple container for a 4-channel floating-point image.

class Image
{
public:
    int width = 1, height = 1;
    vector<Vec4f> rgba;

    void Alloc(int width, int height);
    Vec4f &ptr(int x, int y);
    const Vec4f &ptr(int x, int y) const { return const_cast<Image *>(this)->ptr(x, y); }
    void TopLeftVisiblePixel(int &x, int &y) const;
    void BottomRightVisiblePixel(int &x, int &y) const;
    void CenterVisiblePixel(int &x, int &y) const;
};

void swap(Image &lhs, Image &rhs);

#endif

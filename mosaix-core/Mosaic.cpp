#include "mosaic.h"
#include <math.h>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

bool Mosaic::Options::operator==(const Options &rhs) const
{
    return
        block_size == rhs.block_size &&
        angle == rhs.angle &&
        origin_x == rhs.origin_x &&
        origin_y == rhs.origin_y;
}

// This is like a very small subset of vector<>, but with automatic
// allocation of out-of-range accesses.  Negative indices are allowed.
// We expect data to be relatively contiguous and allocate the entire
// range that's been accessed (this isn't a map), and always allocate
// around 0.
template<typename T>
class autovector
{
public:
    autovector(T default_value = T())
    {
        this->default_value = default_value;
    }
    // Make sure element n is allocated.
    void check(int n)
    {
        if(n >= start && n < end)
            return;

        if(n < start)
        {
            int elements_to_add_at_beginning = start - n;
            data.insert(data.begin(), elements_to_add_at_beginning, default_value);
            start = n;
        }
        else
        {
            int elements_to_add_at_end = n - end + 1;
            data.insert(data.end(), elements_to_add_at_end, default_value);
            end = n + 1;
        }
    }

    T &operator[](int n)
    {
        check(n);
        return data[n - start];
    }

    int start = 0, end = 0;
private:
    T default_value;
    vector<T> data;
};

// Map from pixels in the image to buckets to combine, and handle
// rotation and other transformations.
class ColorBuckets
{
public:
    ColorBuckets(float block_size_, float angle_, int origin_x_, int origin_y_):
        buckets(autovector<Vec4f>(Vec4f(0,0,0,0))),
        block_size(max(1.0f, block_size_)),
        origin_x(origin_x_), origin_y(origin_y_),
        angle(-float(angle_ / 180 * M_PI))
    {
    }

    pair<float,float> get_bucket_coord(int x, int y)
    {
        x -= origin_x;
        y -= origin_y;

        // Rotate the position.
        float result_x = cosf(angle)*x - sinf(angle)*y;
        float result_y = cosf(angle)*y + sinf(angle)*x;

        result_x /= block_size;
        result_y /= block_size;
        return make_pair(result_x, result_y);
    }

    Vec4f &get_bucket(int x, int y)
    {
        pair<float,float> coord = get_bucket_coord(x, y);
        x = int(floorf(coord.first));
        y = int(floorf(coord.second));
        return buckets[x][y];
    };

    autovector<autovector<Vec4f>> buckets;
    float block_size = 1;
    float angle = 0;
    int origin_x = 0, origin_y = 0;
};

namespace Mosaic
{
    void ApplyMosaic(Image &image, const Options &options)
    {
        // Break the image up into buckets, and sum the color in each bucket.
        ColorBuckets color_buckets(options.block_size, options.angle, options.origin_x, options.origin_y);

        for(int y = 0; y < image.height; y++)
        {
            for(int x = 0; x < image.width; x++)
            {
                Vec4f &color = color_buckets.get_bucket(x, y);

                // Sum the color in this block.  The data is premultiplied, so this
                // will weight by alpha, making transparent pixels contribute less to
                // the color of the block than opaque ones.
                color += image.rgba[y*image.width + x];
            }
        }

        // Except for completely transparent buckets, make all buckets completely opaque.
        for(int bucket_y = color_buckets.buckets.start; bucket_y < color_buckets.buckets.end; ++bucket_y)
        {
            autovector<Vec4f> &row_buckets = color_buckets.buckets[bucket_y];
            for(int bucket_x = row_buckets.start; bucket_x < row_buckets.end; ++bucket_x)
            {
                Vec4f &color = row_buckets[bucket_x];
                if(color.w < 0.01)
                    color = Vec4f(0,0,0,0);
                else
                    color *= 1.0f/color.w;
            }
        }

        // Copy the color from the buckets back to the image.
        for(int y = 0; y < image.height; y++)
        {
            for(int x = 0; x < image.width; x++)
            {
                // Write the pixel from its color bucket.  Leave the alpha value in the destination
                // alone, and multiply the color by alpha since our color is premultiplied.
                Vec4f color = color_buckets.get_bucket(x, y);
                Vec4f &output = image.rgba[y*image.width + x];
                output.x = color.x * output.w;
                output.y = color.y * output.w;
                output.z = color.z * output.w;
            }
        }
    }
};
#include "ImageIO.h"

#include <algorithm>
using namespace std;

#include <png.h>
#include <zlib.h>

#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfChannelList.h>
using namespace Imf;
using namespace Imath;

static string get_extension(string filename)
{
    size_t dot = filename.rfind('.');
    if(dot == string::npos)
        return "";

    return filename.substr(dot+1);
}

void ImageHelpers::ReadImage(Image &image, string filename)
{
    if(!stricmp(get_extension(filename).c_str(), "exr"))
        ImageHelpers::ReadEXR(image, filename);
    else
        ImageHelpers::ReadPNG(image, filename);
}

namespace
{
    void setup_png(bool read, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn, png_structp &png, png_infop &info)
    {
        if(read)
            png = ::png_create_read_struct(PNG_LIBPNG_VER_STRING, error_ptr, error_fn, warn_fn);
        else
            png = ::png_create_write_struct(PNG_LIBPNG_VER_STRING, error_ptr, error_fn, warn_fn);
        if(!png)
            throw bad_alloc();

        info = png_create_info_struct(png);
        if(!info)
            throw bad_alloc();
    }
}

void ImageHelpers::ReadPNG(Image &image, string filename)
{
    FILE *f = fopen(filename.c_str(), "rb");
    if(f == NULL)
        throw runtime_error("Error opening " + filename + ": " + strerror(errno));

    png_structp png;
    png_infop info;
    setup_png(true, NULL, NULL, NULL, png, info);

    if(setjmp(png_jmpbuf(png)))
        throw runtime_error("Error reading PNG");

    png_init_io(png, f);
    png_read_info(png, info);

    // Convert everything to 8-bit RGBA.
    png_set_expand(png);
    png_set_strip_16(png);
    png_set_packing(png);
    png_set_palette_to_rgb(png);
    png_set_tRNS_to_alpha(png);
    png_set_gray_to_rgb(png);
    png_set_filler(png, 0xff, PNG_FILLER_AFTER);
    png_set_interlace_handling(png);
    png_read_update_info(png, info);

    png_uint_32 width, height;
    int depth, type;
    png_get_IHDR(png, info, &width, &height, &depth, &type, NULL, NULL, NULL);
    vector<png_byte> data(width*height*4);

    // libpng wants a pointer to each row.
    vector<png_byte *> rows(height);
    for(unsigned y = 0; y < height; ++y)
        rows[y] = &data[y*width*4];

    png_read_image(png, rows.data());

    // Convert the image to an Image.
    image.width = width;
    image.height = height;
    image.rgba.resize(image.height*image.width);

    for(int y = 0; y < (int) height; ++y)
    {
        for(int x = 0; x < (int) width; ++x)
        {
            Vec4f &output = image.ptr(x, y);

            const png_byte *input = &rows[y][x*4];
            for(int c = 0; c < 4; ++c)
                output[c] = input[c] / 255.0f;

            // Premultiply:
            for(int c = 0; c < 3; ++c)
                output[c] *= output.w;
        }
    }

    png_read_end(png, info);
    png_destroy_read_struct(&png, &info, NULL);
}

void ImageHelpers::WritePNG(const Image &image, string filename, bool compression)
{
    FILE *f = fopen(filename.c_str(), "wb");
    if(f == NULL)
        throw runtime_error("Error opening " + filename + ": " + strerror(errno));

    png_structp png;
    png_infop info;
    setup_png(false, NULL, NULL, NULL, png, info);

    if(setjmp(png_jmpbuf(png)))
        throw runtime_error("Error writing PNG");

    png_init_io(png, f);
    png_set_IHDR(png, info, image.width, image.height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_DEFAULT);
    if(!compression)
        png_set_compression_level(png, Z_NO_COMPRESSION);
    png_write_info(png, info);

    // Copy out the image to write it.
    basic_string<uint8_t> buf;
    buf.resize(image.width*4);
    for(int y = 0; y < image.height; y++)
    {
        for(int x = 0; x < image.width; ++x)
        {
            int offset = y*image.width + x;
            float alpha = image.rgba[offset].w;
            uint8_t *output = &buf[x*4];

            for(int c = 0; c < 4; ++c)
            {
                float value = image.rgba[offset][c];

                // Unpremultiply and convert to 8-bit.
                if(c != 3 && alpha > 0.000001f)
                    value /= alpha;

                if(value < 0) value = 0;
                if(value > 1) value = 1;
                output[c] = uint8_t(lrintf(value * 255.0f));
            }
        }
        png_write_row(png, buf.data());
    }

    png_write_end(png, NULL);

    fclose(f);
}

void ImageHelpers::WriteImage(const Image &image, string filename, bool compression)
{
    if(!stricmp(get_extension(filename).c_str(), "exr"))
        ImageHelpers::WriteEXR(image, filename, compression);
    else
        ImageHelpers::WritePNG(image, filename, compression);
}

void ImageHelpers::ReadEXR(Image &image, string filename)
{
    InputFile input_file(filename.c_str());
    Header header = input_file.header();
    Box2i dw = header.dataWindow();
    image.width = dw.max.x - dw.min.x + 1;
    image.height = dw.max.y - dw.min.y + 1;
    image.rgba.resize(image.height*image.width);

    FrameBuffer input_framebuffer;
    input_framebuffer.insert("R", Slice(FLOAT, (char *) &image.rgba[0].x, sizeof(V4f), sizeof(V4f) * image.width));
    input_framebuffer.insert("G", Slice(FLOAT, (char *) &image.rgba[0].y, sizeof(V4f), sizeof(V4f) * image.width));
    input_framebuffer.insert("B", Slice(FLOAT, (char *) &image.rgba[0].z, sizeof(V4f), sizeof(V4f) * image.width));
    input_framebuffer.insert("A", Slice(FLOAT, (char *) &image.rgba[0].w, sizeof(V4f), sizeof(V4f) * image.width));

    input_file.setFrameBuffer(input_framebuffer);
    input_file.readPixels(dw.min.y, dw.max.y);
}

void ImageHelpers::WriteEXR(const Image &image, string filename, bool compression)
{
    Header header(image.width, image.height);
    header.compression() = compression? PIZ_COMPRESSION:NO_COMPRESSION;

    FrameBuffer framebuffer;
    for(string color: { "R", "G", "B", "A" })
        header.channels().insert(color.c_str(), Channel(FLOAT));
    framebuffer.insert("R", Slice(FLOAT, (char *) &image.rgba[0].x, sizeof(V4f), sizeof(V4f) * image.width));
    framebuffer.insert("G", Slice(FLOAT, (char *) &image.rgba[0].y, sizeof(V4f), sizeof(V4f) * image.width));
    framebuffer.insert("B", Slice(FLOAT, (char *) &image.rgba[0].z, sizeof(V4f), sizeof(V4f) * image.width));
    framebuffer.insert("A", Slice(FLOAT, (char *) &image.rgba[0].w, sizeof(V4f), sizeof(V4f) * image.width));

    OutputFile output_file(filename.c_str(), header);
    output_file.setFrameBuffer(framebuffer);
    output_file.writePixels(image.height);
}

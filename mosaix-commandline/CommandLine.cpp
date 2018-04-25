#include <stdexcept>
#include <algorithm>
#include "../mosaix-core/Mosaic.h"
#include "ImageIO.h"
using namespace std;

int main(int argc, char *argv[])
{
    // Compressing the output file can take a good portion of the overall processing time.
    // Allow disabling it for batch use.
    bool enable_compression = true;
    if(argc > 1 && !strcmp(argv[1], "-n"))
    {
        enable_compression = false;
        ++argv;
        --argc;
    }

    if(argc != 4)
    {
        printf("Usage: %s [-n] input.exr output.exr block_size\n", argv[0]);
        return 1;
    }

    string input_filename = argv[1];
    string output_filename = argv[2];
    try {
        // Read the image.
        Image image;

        int block_size = (int) atoi(argv[3]);
        if(block_size == 0)
        {
            printf("Invalid block size\n");
            exit(1);
        }

        ImageHelpers::ReadImage(image, input_filename);

        // Apply the mosaic.
        Mosaic::Options options;
        options.block_size = block_size;
        Mosaic::ApplyMosaic(image, options);
        
        // Write the result.
        ImageHelpers::WriteImage(image, output_filename, enable_compression);
    } catch(exception &e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }
    return 0;
}

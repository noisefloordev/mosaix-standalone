#include <stdexcept>
#include <algorithm>
#include "getopt.h"
#include "../mosaix-core/Mosaic.h"
#include "ImageIO.h"
using namespace std;

void usage(string name)
{
    printf("Usage: %s [-b block-size] [-x x-offset] [-y y-offset] [-a angle] [-n] input.exr output.exr\n", name.c_str());
}

int main(int argc, char *argv[])
{
    // Compressing the output file can take a good portion of the overall processing time.
    // Allow disabling it for batch use.
    bool enable_compression = true;

    Mosaic::Options options;
    while(1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
            {"block-size",      required_argument, 0,  'b' },
            {"no-compression",  no_argument,       0,  'n' },
            {"angle",           required_argument, 0,  'a' },
            {"offset-x",        required_argument, 0,  'x' },
            {"offset-y",        required_argument, 0,  'y'},
            {0,                 0,                 0,  0 }
        };

        int c = getopt_long(argc, argv, "b:nha:x:y:", long_options, &option_index);
        if(c == -1)
            break;

        switch (c) {
        case 'n':
            enable_compression = false;
            break;

        case 'a':
            options.angle = (float) atof(optarg);
            break;

        case 'x':
            options.origin_x = atoi(optarg);
            break;

        case 'y':
            options.origin_y = atoi(optarg);
            break;

        case 'b':
            options.block_size = atoi(optarg);

            if(options.block_size == 0)
            {
                printf("Invalid block size\n");
                exit(1);
            }

            break;

        case 'h':
            usage(argv[0]);
            exit(1);
            break;

        default:
            break;
        }
    }

    if(optind+2 != argc)
    {
        usage(argv[0]);
        return 1;
    }

    string input_filename = argv[optind+0];
    string output_filename = argv[optind+1];
    try {
        // Read the image.
        Image image;

        ImageHelpers::ReadImage(image, input_filename);

        // Apply the mosaic.
        Mosaic::ApplyMosaic(image, options);
        
        // Write the result.
        ImageHelpers::WriteImage(image, output_filename, enable_compression);
    } catch(exception &e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }
    return 0;
}

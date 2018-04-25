#include "PreviewRenderer.h"
#include "PhotoshopHelpers.h"

#include <algorithm>
using namespace std;

#include "../mosaix-core/Mosaic.h"

namespace
{
    void ConvertToBGRX(shared_ptr<const Image> image, vector<uint32_t> &output)
    {
        output.resize(image->width*image->height, 0);
        uint32_t *data = output.data();
        for(int y = 0; y < image->height; ++y)
        {
            for(int x = 0; x < image->width; ++x)
            {
                Vec4f c = image->ptr(x, y);

                // The input color is premultiplied.  Leave it that way, since we're not
                // blending the preview and this fades alpha against black.
                uint8_t r = uint8_t(clamp(c.x * 255.0f, 0.0f, 255.0f));
                uint8_t g = uint8_t(clamp(c.y * 255.0f, 0.0f, 255.0f));
                uint8_t b = uint8_t(clamp(c.z * 255.0f, 0.0f, 255.0f));
                uint32_t result =
                    (b << 0) |
                    (g << 8) |
                    (r << 16) |
                    (0xFF << 24);

                *(data++) = result;
            }
        }
    }
}

void PreviewRenderer::SetSourceImage(shared_ptr<const Image> NewSourceImage)
{
    SourceImage = NewSourceImage;
    ConvertToBGRX(SourceImage, SourceImage8BPP);

    CurrentPreview = make_shared<Image>();
}

void PreviewRenderer::UpdatePreview()
{
    if(CurrentSettings == AppliedSettings)
        return;
    AppliedSettings = CurrentSettings;

    // Run the filter.
	*CurrentPreview = *SourceImage;
    Mosaic::ApplyMosaic(*CurrentPreview.get(), CurrentSettings);

    // Convert to 8-bit RGBA for the preview.
    ConvertToBGRX(CurrentPreview, CurrentPreview8BPP);
}

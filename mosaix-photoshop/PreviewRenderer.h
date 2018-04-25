#ifndef PREVIEW_RENDERER_H
#define PREVIEW_RENDERER_H

#include "../mosaix-core/Image.h"
#include "../mosaix-core/Mosaic.h"

#include <memory>
using namespace std;

struct PreviewRenderer
{
    void SetSourceImage(shared_ptr<const Image> SourceImage);

	void UpdatePreview();

    /* Unprocessed image. */
    shared_ptr<const Image> SourceImage;
    vector<uint32_t> SourceImage8BPP;

    /* Processed preview image and original.  iX and iY will be <= iPreviewX, iPreviewY. */
    shared_ptr<Image> CurrentPreview;
    vector<uint32_t> CurrentPreview8BPP;

	/* The current settings in the UI.  These aren't necessarily applied. */
    Mosaic::Options CurrentSettings;

private:
	/* The options which are actually applied, and the final results. */
	Mosaic::Options AppliedSettings;
};
#endif

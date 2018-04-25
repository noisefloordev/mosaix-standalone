#include "PIDefines.h"
#include "PITypes.h"
#include "PIAbout.h"
#include "PIFilter.h"
#include "PIUtilities.h"
#include "PhotoshopHelpers.h"
#include "Settings.h"
#include "UI.h"

#include "../mosaix-core/Mosaic.h"

#include <stdio.h>
#include <string>
using namespace std;

SPBasicSuite *sSPBasic;

void DoAbout(AboutRecord *pAboutRecord)
{
    PlatformData *pPlatform = (PlatformData *) pAboutRecord->platformData;
    HWND hWnd = (HWND) pPlatform->hwnd;
    MessageBox(hWnd, "Mosaix", "Mosaix", MB_OK);
}

class PhotoshopErrorException: public exception
{
public:
    PhotoshopErrorException(int iErr) { m_iErr = iErr; }

    int m_iErr;
};

// Return the channels we're working with:
// first_channel through first_channel + color_channels
// and alpha_channel if it's not -1.
static void GetColorChannels(const FilterRecord *pFilterRecord, bool input,
    int &first_channel, int &color_channels, int &alpha_channel)
{
    int layerPlanes = input? pFilterRecord->inLayerPlanes:pFilterRecord->outLayerPlanes;
    int transparencyMask = input? pFilterRecord->inTransparencyMask:pFilterRecord->outTransparencyMask;
    int layerMasks = input? pFilterRecord->inLayerMasks:pFilterRecord->outLayerMasks;
    int invertedLayerMasks = input? pFilterRecord->inInvertedLayerMasks:pFilterRecord->outInvertedLayerMasks;
    int nonLayerPlanes = input? pFilterRecord->inNonLayerPlanes:pFilterRecord->outNonLayerPlanes;

    // Data comes in this order:
    //
    // layerPlanes;
    // transparencyMask;
    // layerMasks;
    // invertedLayerMasks;
    // nonLayerPlanes;
    //
    // The documentation and API is very poor: it's all one big chunk for
    // a bunch of applications that use different pieces of it, so it's hard
    // to tell what is actually used by Photoshop.  LayerMasks seems to either
    // not be used or only used if filtersLayerMasks is set.  Since I can't find
    // any case that uses this to test it, it's currently ignored entirely.
    //
    // Apply to regular color channels if possible.  If we don't have
    // any, but we do have inNonLayerPlanes, we're probably being applied
    // to a layer mask, so use those instead.
    if(pFilterRecord->inLayerPlanes > 0)
    {
        // We're filtering a color channel, optionally with a transparent mask (aka alpha).
        first_channel = 0;
        color_channels = layerPlanes;
        alpha_channel = transparencyMask > 0? layerPlanes:-1;

        // If we do this, we'll use a layer mask if available.  This is interesting
        // with smart layers, since you can draw a mask on top of the smart layer
        // and we'll use the mask for visibility.  It's a little wonky: if you
        // modify the mask the smart filter doesn't update unless you disable and
        // reenable the filter.  We'd also need to apply both the layer mask and
        // alpha so we don't ignore real alpha.
        //
        // You can get a similar effect by putting the mask inside the smart layer,
        // though with linked images that's a little cumbersome.
        //
        //alpha_channel = layerMasks > 0? (layerPlanes+transparencyMask):
        //    transparencyMask > 0? layerPlanes:-1;
    }
    else
    {
        // We're not filtering color, so we're probably filtering a layer mask,
        // which is in nonLayerPlanes.
        // The offset to the first inNonLayerPlane.  This is usually 0, since we
        // only seem to get this if we have a layer mask selected.
        first_channel = layerPlanes + transparencyMask + layerMasks + invertedLayerMasks;
        color_channels = nonLayerPlanes;
        alpha_channel = -1;
    }

    // We don't support more than 3 color channels (eg. CMYK).
    color_channels = min(color_channels, 3);
}

int GetPhotoshopColorIdx(const FilterRecord *pFilterRecord, bool input, int x, int y, int channel)
{
    int bytes_per_channel = pFilterRecord->depth/8;
    int rowBytes = input? pFilterRecord->inRowBytes:pFilterRecord->outRowBytes;
    int planes = (input? pFilterRecord->inHiPlane:pFilterRecord->outHiPlane) + 1;
    int idx = x*planes*bytes_per_channel + y*rowBytes;
    idx += channel*bytes_per_channel;
    return idx;
}

float GetPhotoshopColor(const FilterRecord *pFilterRecord, int x, int y, int channel)
{
    const uint8_t *data = (const uint8_t *) pFilterRecord->inData;
    data += GetPhotoshopColorIdx(pFilterRecord, true /* input */, x, y, channel);

    if(pFilterRecord->depth == 8)
        return *data / 255.0f;

    if(pFilterRecord->depth == 16)
    {
        const uint16_t *data16 = (const uint16_t *) data;
        return *data16 / 65535.0f;
    }

    if(pFilterRecord->depth == 32)
    {
        const float *data32 = (const float *) data;
        return *data32;
    }

    return 0;
}

void SetPhotoshopColor(const FilterRecord *pFilterRecord, int x, int y, int channel, float value)
{
    uint8_t *data = (uint8_t *) pFilterRecord->outData;
    data += GetPhotoshopColorIdx(pFilterRecord, false /* output */, x, y, channel);

    if(pFilterRecord->depth == 8)
    {
        *data = uint8_t(value * 255.0);
        return;
    }

    if(pFilterRecord->depth == 16)
    {
        uint16_t *data16 = (uint16_t *) data;
        *data16 = uint16_t(value * 65535.0f);
        return;
    }

    if(pFilterRecord->depth == 32)
    {
        float *data32 = (float *) data;
        *data32 = value;
        return;
    }
}

void CopyFromPhotoshop(const FilterRecord *pFilterRecord, shared_ptr<Image> out)
{
    int first_channel, color_channels, alpha_channel;
    GetColorChannels(pFilterRecord, true, first_channel, color_channels, alpha_channel);

    // Allocate the image.
    VPoint size = pFilterRecord->bigDocumentData->imageSize32;
    out->width = size.h;
    out->height = size.v;
    out->rgba.resize(out->width * out->height);

    const uint8_t *photoshop_image = (uint8_t *) pFilterRecord->inData;
    int bytes_per_channel = pFilterRecord->depth/8;
    int stride_bytes = pFilterRecord->inRowBytes;
    for(int y = 0; y < out->height; ++y)
    {
        for(int x = 0; x < out->width; ++x)
        {
            Vec4f &color = out->rgba[y*out->width+x];

            int in_offset = x*pFilterRecord->planes*bytes_per_channel + y*stride_bytes;
            const uint8_t *d = &photoshop_image[in_offset];

            // If we have no alpha channel, set it to 1.
            if(alpha_channel != -1)
                color.w = GetPhotoshopColor(pFilterRecord, x, y, alpha_channel);
            else
                color.w = 1;

            for(int c = 0; c < color_channels; ++c)
                color[c] = GetPhotoshopColor(pFilterRecord, x, y, c);

            if(alpha_channel != -1)
            {
                // Premultiply alpha.
                for(int c = 0; c < 3; ++c)
                    color[c] *= color.w;
            }
        }
    }
}

void CopyToPhotoshop(FilterRecord *pFilterRecord, shared_ptr<const Image> image)
{
    int first_channel, color_channels, alpha_channel;
    GetColorChannels(pFilterRecord, false, first_channel, color_channels, alpha_channel);

    int bytes_per_channel = pFilterRecord->depth/8;
    int stride_bytes = pFilterRecord->outRowBytes;

    uint8_t *photoshop_image = (uint8_t *) pFilterRecord->outData;

    for(int y = 0; y < image->height; ++y)
    {
        for(int x = 0; x < image->width; ++x)
        {
            const Vec4f &color = image->rgba[y*image->width+x];
            int in_offset = x*pFilterRecord->planes*bytes_per_channel + y*stride_bytes;
            uint8_t *d = &photoshop_image[in_offset];

            // We don't modify alpha, so we don't have to copy it out here.
            float alpha = color.w;
            for(int c = 0; c < color_channels; ++c)
            {
                float value = color[c];

                // Unpremultiply alpha.
                if(alpha > 0.001f)
                    value /= alpha;

                SetPhotoshopColor(pFilterRecord, x, y, c, value);
            }
        }
    }
}

// Return the total number of input or output planes.
int GetTotalPlanes(const FilterRecord *pFilterRecord, bool input)
{
    int layerPlanes =        input? pFilterRecord->inLayerPlanes:       pFilterRecord->outLayerPlanes;
    int transparencyMask =   input? pFilterRecord->inTransparencyMask:  pFilterRecord->outTransparencyMask;
    int layerMasks =         input? pFilterRecord->inLayerMasks:        pFilterRecord->outLayerMasks;
    int invertedLayerMasks = input? pFilterRecord->inInvertedLayerMasks:pFilterRecord->outInvertedLayerMasks;
    int nonLayerPlanes =     input? pFilterRecord->inNonLayerPlanes:    pFilterRecord->outNonLayerPlanes;
    return layerPlanes + transparencyMask + layerMasks + invertedLayerMasks + nonLayerPlanes;
}

void InitSourceImage(FilterRecord *pFilterRecord)
{
    VPoint size = pFilterRecord->bigDocumentData->imageSize32;
    VRect filterRect = { 0, 0, size.v, size.h };
    pFilterRecord->bigDocumentData->inRect32 = filterRect;
    pFilterRecord->bigDocumentData->outRect32 = filterRect;
    pFilterRecord->bigDocumentData->maskRect32 = filterRect;
    pFilterRecord->inputRate = (int32)1 << 16; // 1
    pFilterRecord->maskRate = (int32)1 << 16; // 1
    pFilterRecord->inLoPlane = 0;
    pFilterRecord->inHiPlane = GetTotalPlanes(pFilterRecord, true) - 1;
    pFilterRecord->outLoPlane = 0;
    pFilterRecord->outHiPlane = GetTotalPlanes(pFilterRecord, false) - 1;
    pFilterRecord->inputPadding = 255;
    pFilterRecord->maskPadding = 255;
    pFilterRecord->wantsAbsolute = true;

    int iRet = pFilterRecord->advanceState();
    if(iRet != noErr)
    {
        printf("advanceState error %i\n", iRet);
        throw PhotoshopErrorException(iRet);
    }
}

static void Run(FilterRecord *pFilterRecord)
{
    // Set up the input and output buffers.
    InitSourceImage(pFilterRecord);

    // Copy the data from outData to our image.
    shared_ptr<Image> image = make_shared<Image>();
    CopyFromPhotoshop(pFilterRecord, image);

    Mosaic::Options options;
    ReadRegistryParameters(options);
    ReadScriptParameters(options);

    if(pFilterRecord->descriptorParameters->playInfo == plugInDialogDisplay)
    {
        bool ret = DoUI(options, image);

        if(!ret)
            throw PhotoshopErrorException(userCanceledErr);

        WriteRegistryParameters(options);
    }

    WriteScriptParameters(options);

    // Apply the mosaic.
    Mosaic::ApplyMosaic(*image.get(), options);

    // Copy out the result.
    CopyToPhotoshop(pFilterRecord, image);
}

static int16 RunPlugin(FilterRecord *pFilterRecord, const int16 iSelector)
{
    string error;
    try
    {
        pFilterRecord->bigDocumentData->PluginUsing32BitCoordinates = true;

        switch(iSelector)
        {
        case filterSelectorParameters:	break;
        case filterSelectorPrepare:	break;
        case filterSelectorStart:
        {
            Run(pFilterRecord);
            break;
        }
        case filterSelectorContinue:
            pFilterRecord->bigDocumentData->inRect32 = { 0, 0, 0, 0 };
            pFilterRecord->bigDocumentData->outRect32 = { 0, 0, 0, 0 };
            pFilterRecord->bigDocumentData->maskRect32 = { 0, 0, 0, 0 };
            break;
        case filterSelectorFinish:
            break;
        }
    } catch(const std::bad_alloc &) {
        return memFullErr;
    } catch(const PhotoshopErrorException &e) {
        return e.m_iErr;
    } catch(const exception &e) {
        /* Don't let any exceptions propagate back into Photoshop. */
        error = e.what();
    } catch(...) {
        error = "Unknown exception";
    }

    if(error.empty())
        return noErr;

    PlatformData *pPlatform = (PlatformData *) pFilterRecord->platformData;
    HWND hWnd = (HWND) pPlatform->hwnd;
    MessageBox(hWnd, error.c_str(), "Mosaix", MB_OK);

    // Return userCanceledErr on error so Photoshop doesn't show its own generic error.
    return userCanceledErr;
}

DLLExport MACPASCAL void PluginMain(const int16 iSelector, void *pFilterRecord_, int32 *pData, int16 *pResult)
{
    if((GetKeyState(VK_LCONTROL) & 0x8000) && (GetKeyState(VK_LSHIFT) & 0x8000))
    {
        AllocConsole();
        freopen("CONOUT$", "wb", stdout);
        freopen("CONOUT$", "wb", stderr);
    }

    FilterRecord *pFilterRecord = (FilterRecord *) pFilterRecord_;
    gFilterRecord = pFilterRecord;

    if(iSelector == filterSelectorAbout)
        sSPBasic = ((AboutRecord*)pFilterRecord)->sSPBasic;
    else
        sSPBasic = pFilterRecord->sSPBasic;

    /* Don't allocate any non-POD temporaries in this function; we deallocate our heap in MemoryFree. */
    if(iSelector == filterSelectorAbout)
        DoAbout((AboutRecord*) pFilterRecord);
    else
    {
        *pResult = RunPlugin(pFilterRecord, iSelector);
    }
}

extern "C" BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    g_hInstance = (HINSTANCE) hModule;
    return true;
}

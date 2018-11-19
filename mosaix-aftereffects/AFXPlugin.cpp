#include <string>
#include <algorithm>
#include <memory>
using namespace std;

#include "../mosaix-core/Mosaic.h"

#include <stdlib.h>
#include <string.h>

#include "AE_Effect.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AEConfig.h"
#include "AEGP_SuiteHandler.h"
#include "AEFX_SuiteHelper.h"
#include "entry.h"

#define NAME "Mosaix"
#define DESCRIPTION "Clean, flexible image mosaics"
#define MAJOR_VERSION 3
#define MINOR_VERSION 1
#define BUG_VERSION 0
#define STAGE_VERSION PF_Stage_DEVELOP
#define BUILD_VERSION 1

// XXX: support 16- and 32-bit color

static AEGP_PluginID plugin_id;

class AFXErrorException: public exception
{
public:
    AFXErrorException(PF_Err iErr, string msg_ = "")
    {
        m_iErr = iErr;
        msg = msg_;
    }

    PF_Err m_iErr;
    string msg;
};

template<typename T>
struct Suite
{
    Suite(const PF_InData *in_data_, const char *name_, int version_):
        in_data(in_data_),
        name(name_),
        version(version_)
    {
        PF_Err err = in_data->pica_basicP->AcquireSuite(name, version, (const void**)&p);
        if(err)
            throw AFXErrorException(err);
    }

    ~Suite()
    {
        if(p == NULL)
            return;

        // Don't throw errors from a dtor.  If After Effects gives us an error releasing
        // a suite, that's its problem.
        in_data->pica_basicP->ReleaseSuite(name, version);
    }

    const PF_InData *in_data;
    const char *name;
    int version;
    T *p = NULL;
};

enum {
    Param_BlockSize = 1,
    Param_Angle,
    Param_Offset,
    Param_Mask,
    Param_MaskOffset,
    Param_Count,
};

// Check out a parameter when this class goes out of scope.
struct CheckOutParam
{
    CheckOutParam(const PF_InData *in_data_, PF_ParamDef &def_):
        in_data(in_data_),
        def(def_)
    {
    }

    ~CheckOutParam()
    {
        in_data->inter.checkin_param(in_data->effect_ref, &def);
    }

    const PF_InData *in_data;
    PF_ParamDef &def;
};

static void About(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg, "%s, v%d.%d\r%s", NAME, MAJOR_VERSION, MINOR_VERSION, DESCRIPTION);
}

static void GlobalSetup(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
    out_data->my_version = PF_VERSION(MAJOR_VERSION, MINOR_VERSION, BUG_VERSION, STAGE_VERSION, BUILD_VERSION);
    out_data->out_flags = 0;
    out_data->out_flags2 = 0;

    AEGP_SuiteHandler suites(in_data->pica_basicP);

    PF_Err err = suites.UtilitySuite3()->AEGP_RegisterWithAEGP(NULL, "Plugin", &plugin_id);
    if(err)
        throw AFXErrorException(err);
}

static PF_Err ParamsSetup(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
    PF_ParamDef def;

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDER("BlockSize", 1, 1000, 1, 100, 0 /* unused */, 10, 1, 0, 0, Param_BlockSize);

    AEFX_CLR_STRUCT(def);
    PF_ADD_ANGLE("Angle", 0, Param_Angle);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POINT("Offset", 50, 50, false, Param_Offset);

    AEFX_CLR_STRUCT(def);
    PF_ADD_LAYER("Mask", 0, Param_Mask);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POINT("MaskOffset", 50, 50, false, Param_MaskOffset);
    
    out_data->num_params = Param_Count;
    
    return PF_Err_NONE;
}

inline PF_Pixel vec4_to_pfpixel(const Vec4f &color)
{
    auto convert = [](float c)
    {
        c *= 255.0f;
        return (uint8_t) max(min(lrintf(c), 255L), 0L);
    };

    PF_Pixel result;
    result.red = convert(color.x);
    result.green = convert(color.y);
    result.blue = convert(color.z);
    result.alpha = convert(color.w);
    return result;
}

static PF_PixelFormat GetPixelFormat(const PF_InData *in_data, const PF_LayerDef *image)
{
    // If we used more suites or if this was called more often it'd be worth
    // being smarter about allocating suites, but for now this is fine.
    Suite<PF_WorldSuite2> world(in_data, kPFWorldSuite, kPFWorldSuiteVersion2);

    PF_PixelFormat result;
    PF_Err err = world.p->PF_GetPixelFormat(image, &result);
    if(err)
        throw AFXErrorException(err);

    return result;
}

void CopyFromAfterEffects_8BPP(const PF_InData *in_data, PF_LayerDef *layer, shared_ptr<Image> result)
{
    PF_Pixel8 *in_pixel_data = NULL;
    PF_Err err = in_data->utils->get_pixel_data8(layer, NULL, &in_pixel_data);
    if(err)
        throw AFXErrorException(err);

    const char *in_pixel_data_p = (const char *) in_pixel_data;
    for(int y = 0; y < result->height; ++y)
    {
        const PF_Pixel *in_row = (PF_Pixel8 *) &in_pixel_data_p[y*layer->rowbytes];
        for(int x = 0; x < result->width; ++x)
        {
            Vec4f &out_pixel = result->rgba[y*result->width+x];
            const PF_Pixel &in_pixel = in_row[x];

            out_pixel.x = in_pixel.red / 255.0f;
            out_pixel.y = in_pixel.green / 255.0f;
            out_pixel.z = in_pixel.blue / 255.0f;
            out_pixel.w = in_pixel.alpha / 255.0f;
        }
    }
}

void CopyFromAfterEffects_16BPP(const PF_InData *in_data, PF_LayerDef *layer, shared_ptr<Image> result)
{
    PF_Pixel16 *in_pixel_data = NULL;
    PF_Err err = in_data->utils->get_pixel_data16(layer, NULL, &in_pixel_data);
    if(err)
        throw AFXErrorException(err);

    const uint16_t *in_pixel_data_p = (const uint16_t *) in_pixel_data;
    for(int y = 0; y < result->height; ++y)
    {
        const PF_Pixel *in_row = (PF_Pixel8 *) &in_pixel_data_p[y*layer->rowbytes];
        for(int x = 0; x < result->width; ++x)
        {
            Vec4f &out_pixel = result->rgba[y*result->width+x];
            const PF_Pixel &in_pixel = in_row[x];

            out_pixel.x = in_pixel.red / 65535.0f;
            out_pixel.y = in_pixel.green / 65535.0f;
            out_pixel.z = in_pixel.blue / 65535.0f;
            out_pixel.w = in_pixel.alpha / 65535.0f;
        }
    }
}

shared_ptr<Image> CopyFromAfterEffects(const PF_InData *in_data, PF_LayerDef *layer)
{
    shared_ptr<Image> result = make_shared<Image>();
    result->width = layer->width;
    result->height = layer->height;
    result->rgba.resize(result->width * result->height);

    PF_PixelFormat pf = GetPixelFormat(in_data, layer);
    switch(pf)
    {
    case PF_PixelFormat_ARGB32:
        CopyFromAfterEffects_8BPP(in_data, layer, result);
        break;
    // 16-bit support isn't complete and not enabled yet.
    case PF_PixelFormat_ARGB64:
        CopyFromAfterEffects_16BPP(in_data, layer, result);
        break;
    default:
        throw AFXErrorException(PF_Err_BAD_CALLBACK_PARAM, "Unsupported image format");
    }

    // Premultiply the result.
    for(Vec4f &pixel: result->rgba)
    {
        pixel.x *= pixel.w;
        pixel.y *= pixel.w;
        pixel.z *= pixel.w;
    }

    return result;
}

void CopyToAfterEffects_8BPP(PF_InData *in_data, PF_LayerDef *layer, shared_ptr<const Image> image)
{
    PF_Err err = PF_Err_NONE; 

    PF_Pixel8 *out_pixel_data = NULL;
    err = PF_GET_PIXEL_DATA8(layer, NULL, &out_pixel_data);
    if(err)
        throw AFXErrorException(err);

    const char *out_pixel_data_p = (const char *) out_pixel_data;

    for(int y = 0; y < image->height; ++y)
    {
        PF_Pixel *out_row = (PF_Pixel8 *) &out_pixel_data_p[y*layer->rowbytes];
        for(int x = 0; x < image->width; ++x)
        {
            Vec4f in_pixel = image->rgba[y*image->width+x];
            PF_Pixel &out_pixel = out_row[x];

            // Unpremultiply:
            if(in_pixel[3] > 0.001)
            {
                in_pixel[0] /= in_pixel[3];
                in_pixel[1] /= in_pixel[3];
                in_pixel[2] /= in_pixel[3];
            }

            auto convert = [in_pixel](float color)
            {
                color *= 255.0f;
                return (uint8_t) max(min(lrintf(color), 255L), 0L);
            };

            out_pixel = vec4_to_pfpixel(in_pixel);
        }
    }
}

void CopyToAfterEffects(PF_InData *in_data, PF_LayerDef *layer, shared_ptr<const Image> image)
{
    PF_PixelFormat pf = GetPixelFormat(in_data, layer);
    switch(pf)
    {
    case PF_PixelFormat_ARGB32:
        CopyToAfterEffects_8BPP(in_data, layer, image);
        break;
    default:
        throw AFXErrorException(PF_Err_BAD_CALLBACK_PARAM, "Unsupported image format");
    }
}

void ApplyMask(shared_ptr<Image> image, shared_ptr<const Image> mask, int offset_x, int offset_y)
{
    for(int y = 0; y < image->height; ++y)
    {
        for(int x = 0; x < image->width; ++x)
        {
            Vec4f &color = image->rgba[y*image->width+x];
            int mx = min(max(x + offset_x, 0), mask->width-1);
            int my = min(max(y + offset_y, 0), mask->height-1);

            const Vec4f &mask_color = mask->rgba[my*mask->width+mx];

            // We only support monochrome masks, so multiply by red rather than
            // per-channel.
            color *= mask_color.x;
        }
    }
}

shared_ptr<Image> CheckOutAndCopyFromAfterEffects(const PF_InData *in_data, int param)
{
    // Check out the image.
    PF_ParamDef p = {0};
    PF_Err err = in_data->inter.checkout_param(in_data->effect_ref, param, in_data->current_time, in_data->time_step, in_data->time_scale, &p);

    // This handles checking it back in.
    CheckOutParam checkout(in_data, p);
    if(err)
        throw AFXErrorException(err);

    // If data is null, there's no image.
    PF_LayerDef *layer = &p.u.ld;
    if(layer->data == nullptr)
        return nullptr;

    return CopyFromAfterEffects(in_data, layer);
};

static void Render(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
    PF_Err err = PF_Err_NONE; 

    // Read mosaic options.
    Mosaic::Options options;
    options.block_size = float(params[Param_BlockSize]->u.fs_d.value);
    options.angle = float(params[Param_Angle]->u.ad.value / double(0x10000));
    options.origin_x = lrint(params[Param_Offset]->u.td.x_value / double(0x10000));
    options.origin_y = lrint(params[Param_Offset]->u.td.y_value / double(0x10000));

    // If the image is scaled down, we'll be working on downsampled data.
    // Adjust the block size and offsets to compensate.  We don't support
    // non-square block sizes.  Points already have downsampling applied, so
    // we only need to adjust this.
    options.block_size = options.block_size * in_data->downsample_x.num / in_data->downsample_x.den;

    // Read the input image.
    PF_LayerDef *input = &params[0]->u.ld;
    shared_ptr<Image> image = CopyFromAfterEffects(in_data, input);

    // If the block size is 1, nothing will actually happen.  Skip processing and
    // just copy out the input image.
    if(options.block_size <= 1.00001f)
        return CopyToAfterEffects(in_data, output, image);

    // If we have a mask, read it.
    shared_ptr<Image> mask = CheckOutAndCopyFromAfterEffects(in_data, Param_Mask);

    shared_ptr<Image> original_image;
    if(mask)
    {
        // Make a copy of the unmasked image, so we can comp the result over it
        // at the end.
        original_image = make_shared<Image>();
        *original_image = *image;

        // Apply the mask to the image we'll mosaic.
        //
        // If mask_offset is in the center of the image (the default), center the mask
        // in the image.  If the image and mask are the same size, this will overlap them
        // normally.  If mask_offset is 0,0, center the mask in the top-left of the image.
        //
        // With a 0x0 offset, the top-left of the mask will be aligned to the top-
        // left of the image.  Shift the mask to center it on the top-left of the
        // image.
        float offset_x = mask->width / 2.0f;
        float offset_y = mask->height / 2.0f;

        // Shift by the offset.
        float mask_offset_x = params[Param_MaskOffset]->u.td.x_value / float(0x10000);
        float mask_offset_y = params[Param_MaskOffset]->u.td.y_value / float(0x10000);
        offset_x -= mask_offset_x;
        offset_y -= mask_offset_y;

        ApplyMask(image, mask, lrintf(offset_x), lrintf(offset_y));
    }

    // Apply the mosaic.
    Mosaic::ApplyMosaic(*image.get(), options);

    if(original_image)
    {
        // Composite the masked image back over the original.
        original_image->AlphaComposite(image);
        image = original_image;
    }

    // Copy out the result.
    CopyToAfterEffects(in_data, output, image);
}

extern "C" DllExport PF_Err PluginMain(PF_Cmd cmd, PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output, void *extra)
{
    try {
        switch(cmd) {
        case PF_Cmd_ABOUT: About(in_data, out_data, params, output); break;
        case PF_Cmd_GLOBAL_SETUP: GlobalSetup( in_data, out_data, params, output); break;
        case PF_Cmd_PARAMS_SETUP: return ParamsSetup( in_data, out_data, params, output); break;
        case PF_Cmd_RENDER: Render( in_data, out_data, params, output); break;
        default: return PF_Err_NONE;
        }
    } catch(AFXErrorException &e) {
        if(!e.msg.empty())
        {
            out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
            strncpy(out_data->return_msg, e.msg.c_str(), sizeof(out_data->return_msg)-1);
            out_data->return_msg[sizeof(out_data->return_msg)-1] = 0;
        }

        return e.m_iErr;
    }
    return PF_Err_NONE;
}



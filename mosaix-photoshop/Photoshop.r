#include "Constants.h"

#define plugInCopyrightYear    "2018"
#define plugInDescription      "Mosaix"

// This determines the menu item the filter goes into.  This won't work correctly for
// non-English versions of Photoshop, but there's nothing we can do about that.
#define vendorName             "Pixelate"
#define plugInAETEComment      "Mosaix"

#define plugInSuiteID        'sdK4'
#define plugInClassID        'simP'
#define plugInEventID        typeNull

#define Rez
#include "PIDefines.h"
#include "PIGeneral.r"
#include "PIUtilities.r"
#include "PIActions.h"
#include "PITerminology.h"

resource 'PiPL' ( 16000, "Mosaix", purgeable )
{
    {
        Kind { Filter },
        Name { plugInName "..." },
        Category { ""vendorName },
        Version { (latestFilterVersion << 16 ) | latestFilterSubVersion },
        CodeWin64X86 { "PluginMain" },

        SupportedModes
        {
            noBitmap, doesSupportGrayScale,
            noIndexedColor, doesSupportRGBColor,
            noCMYKColor, doesSupportHSLColor,
            doesSupportHSBColor, doesSupportMultichannel,
            doesSupportDuotone, doesSupportLABColor
        },

        HasTerminology
        {
            plugInClassID,
            plugInEventID,
            16000,
            plugInUniqueID
        },
        
        EnableInfo { "in (PSHOP_ImageMode, RGBMode, GrayScaleMode,"
                     "HSLMode, HSBMode, MultichannelMode,"
                     "DuotoneMode, LabMode, RGB48Mode, Gray16Mode) ||"
                     "PSHOP_ImageDepth == 16 ||"
                     "PSHOP_ImageDepth == 32"},

        PlugInMaxSize { 2000000, 2000000 },

        // Undocumented field to enable as a smart filter:
        FilterLayerSupport { doesSupportFilterLayers },

        FilterCaseInfo
        {
            {
                /* Flat data, no selection */
                inStraightData, outStraightData,
                doNotWriteOutsideSelection,
                filtersLayerMasks, doesNotWorkWithBlankData,
                doNotCopySourceToDestination,
                    
                /* Flat data with selection */
                inStraightData, outStraightData,
                doNotWriteOutsideSelection,
                filtersLayerMasks, doesNotWorkWithBlankData,
                doNotCopySourceToDestination,
                
                /* Floating selection */
                inStraightData, outStraightData,
                doNotWriteOutsideSelection,
                filtersLayerMasks, doesNotWorkWithBlankData,
                doNotCopySourceToDestination,
                    
                /* Editable transparency, no selection */
                inStraightData, outStraightData,
                doNotWriteOutsideSelection,
                filtersLayerMasks, doesNotWorkWithBlankData,
                doNotCopySourceToDestination,
                    
                /* Editable transparency, with selection */
                inStraightData, outStraightData,
                doNotWriteOutsideSelection,
                filtersLayerMasks, doesNotWorkWithBlankData,
                doNotCopySourceToDestination,
                    
                /* Preserved transparency, no selection */
                inStraightData, outStraightData,
                doNotWriteOutsideSelection,
                filtersLayerMasks, doesNotWorkWithBlankData,
                doNotCopySourceToDestination,
                    
                /* Preserved transparency, with selection */
                inStraightData, outStraightData,
                doNotWriteOutsideSelection,
                filtersLayerMasks, doesNotWorkWithBlankData,
                doNotCopySourceToDestination
            }
        }    
    }
};

resource 'aete' (16000, "Mosaix", purgeable)
{
    1, 0, english, roman,                                    /* aete version and language specifiers */
    {
        vendorName,                                          /* vendor suite name */
        "Mosaix",                                            /* optional description */
        plugInSuiteID,                                       /* suite ID */
        1,                                                   /* suite code, must be 1 */
        1,                                                   /* suite level, must be 1 */
        {                                                    /* structure for filters */
            plugInName,                                      /* unique filter name */
            plugInAETEComment,                               /* optional description */
            plugInClassID,                                   /* class ID, must be unique or Suite ID */
            plugInEventID,                                   /* event ID, must be unique to class ID */
            
            NO_REPLY,                                        /* never a reply */
            IMAGE_DIRECT_PARAMETER,                          /* direct parameter, used by Photoshop */
            {                                                /* parameters here, if any */
                "block size",                                /* parameter name */
                keyBlockSize,                                /* parameter key ID */
                typeFloat,                                   /* parameter type ID */
                "block size",                                /* optional description */
                flagsSingleParameter,                        /* parameter flags */

                "angle",                                     /* parameter name */
                keyAngle,                                    /* parameter key ID */
                typeFloat,                                   /* parameter type ID */
                "angle",                                     /* optional description */
                flagsSingleParameter,                        /* parameter flags */

                "x offset",                                  /* parameter name */
                keyOffsetX,                                  /* parameter key ID */
                typeInteger,                                 /* parameter type ID */
                "x offset",                                  /* optional description */
                flagsSingleParameter,                        /* parameter flags */

                "y offset",                                  /* parameter name */
                keyOffsetY,                                  /* parameter key ID */
                typeInteger,                                 /* parameter type ID */
                "y offset",                                  /* optional description */
                flagsSingleParameter,                        /* parameter flags */

            }
        },
        {                                                    /* non-filter plug-in class here */
        },
        {                                                    /* comparison ops (not supported) */
        },
        {                                                    /* any enumerations */
        }
    }
};

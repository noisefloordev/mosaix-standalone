#include "AEConfig.h"
#include "AE_EffectVers.h"

resource 'PiPL' (16000)
{
    {
        Kind { AEEffect },
        Name { "Mosaix" },
        Category { "Stylize" },
#if defined(AE_OS_WIN)
        CodeWin64X86 {"PluginMain"},
#elif defined(AE_OS_MAC)
        CodeMachOPowerPC {"PluginMain"},
        CodeMacIntel32 {"PluginMain"},
        CodeMacIntel64 {"PluginMain"},
#endif
        AE_PiPL_Version { 2, 0 },
        AE_Effect_Spec_Version { PF_PLUG_IN_VERSION, PF_PLUG_IN_SUBVERS },
        AE_Effect_Version { 1605633 /* 3.1 */ },
        AE_Effect_Info_Flags { 0 },
        AE_Effect_Global_OutFlags { 0 },
        AE_Effect_Global_OutFlags_2 { 0 },
        AE_Effect_Match_Name { "Mosaix" },
        AE_Reserved_Info { 8 }
    }
};

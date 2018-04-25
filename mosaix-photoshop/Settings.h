#ifndef PLUGIN_SETTINGS_H
#define PLUGIN_SETTINGS_H

#include "../mosaix-core/Mosaic.h"

void ReadScriptParameters(Mosaic::Options &params);
void WriteScriptParameters(const Mosaic::Options &params);
void ReadRegistryParameters(Mosaic::Options &params);
void WriteRegistryParameters(const Mosaic::Options &params);

#endif

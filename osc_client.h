#ifndef OSC_CLIENT_H_INCLUDED
#define OSC_CLIENT_H_INCLUDED

#include "lo/lo.h"

void list_banks();
void list_presets(const int bank_id);
void rescan_presets();
void load_preset(const char *path);
void quit_zyn();

#endif
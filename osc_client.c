#include <stdio.h>
#include <stdlib.h>

#include "lo/lo.h"
#include <string.h>

void list_banks()
{
    lo_address t = lo_address_new(NULL, "7777");

    lo_send(t, "/bank/banks", NULL);

    lo_address_free(t);
}

void list_presets(const int bank_id)
{
    lo_address t = lo_address_new(NULL, "7777");

    lo_send(t, "/bank/bank_select", "i", bank_id);

    lo_address_free(t);
}

void rescan_presets()
{
    lo_address t = lo_address_new(NULL, "7777");

    lo_send(t, "/bank/rescan", NULL);

    lo_address_free(t);
}

void load_preset(const char *path)
{
    lo_address t = lo_address_new(NULL, "7777");

    lo_send(t, "/load-part", "is", 0, path);

    lo_address_free(t);
}

void quit_zyn()
{
    lo_address t = lo_address_new(NULL, "7777");

    lo_send(t, "/quit", NULL);

    lo_address_free(t); 
}
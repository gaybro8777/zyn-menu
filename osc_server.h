#ifndef OSC_SERVER_H_INCLUDED
#define OSC_SERVER_H_INCLUDED

#include "lo/lo.h"

struct zyn_bank {
    int id;
    char *name;
    char *path;

    size_t name_length;
    size_t path_length;
};

struct zyn_preset {
    int id;
    char *name;
    char *path;

    size_t name_length;
    size_t path_length;
};

void run_osc_server();

void osc_server_error(int num, const char *m, const char *path);

int generic_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);
int bank_select_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);
int bankview_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);
int quit_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);

#endif
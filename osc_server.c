#include <stdio.h>
#include <stdlib.h>

#include <osc_server.h>

#include <main.h>
#include <string.h>

int done = 0;
lo_server_thread server_thread;

void run_osc_server()
{
    lo_server_thread server_thread = lo_server_thread_new("7770", osc_server_error);

    lo_server_thread_add_method(server_thread, "/bank/bank_select", "iss", bank_select_handler, NULL);
    lo_server_thread_add_method(server_thread, "/bankview", "iss", bankview_handler, NULL);
    lo_server_thread_add_method(server_thread, "/quit", "", quit_handler, NULL);
    lo_server_thread_add_method(server_thread, NULL, NULL, generic_handler, NULL);

    lo_server_thread_start(server_thread);
}

void osc_server_error(int num, const char *msg, const char *path)
{
    fprintf(stderr, "liblo server error %d in path %s: %s\n", num, path, msg);
}

int generic_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
{
    //fprintf(stderr, "Generic handler: %s %s\n", path, types);

    return 0;
}

int bank_select_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
{
    struct zyn_bank bank;

    char *bank_name = (char*)argv[1];
    char *bank_path = (char*)argv[2];

    bank.id = argv[0]->i;
    bank.name = bank_name;
    bank.path = bank_path;
    
    bank.name_length = strlen(bank_name) + 1;
    bank.path_length = strlen(bank_path) + 1;

    append_bank(bank);

    return 0;
}

int bankview_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
{
    struct zyn_preset preset;

    char *preset_name = (char*)argv[1];
    char *preset_path = (char*)argv[2];

    if(strcmp(preset_name, "\0") == 0)
        return 0;

    preset.id = argv[0]->i;
    preset.name = preset_name;
    preset.path = preset_path;
    
    preset.name_length = strlen(preset_name) + 1;
    preset.path_length = strlen(preset_path) + 1;

    append_preset(preset);

    return 0;
}

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
{
    done = 1;

    printf("quitting\n\n");
    fflush(stdout);
    lo_server_thread_free(server_thread);

    return 0;
}
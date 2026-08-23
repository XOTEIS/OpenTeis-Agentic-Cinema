#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dlfcn.h>
#include <stdio.h>

#define MAX_LIB_NAME 128

typedef struct {
    void* active_handle;
    char active_path[MAX_LIB_NAME];
    uint32_t hot_swaps_performed;
    uint32_t version_generation;
    bool is_hook_active;
} __attribute__((aligned(64))) AutoCodeJitState_t;

__attribute__((visibility("default")))
AutoCodeJitState_t* autocode_jit_init(const char* initial_lib_path) {
    AutoCodeJitState_t* st = NULL;
    if (posix_memalign((void**)&st, 64, sizeof(AutoCodeJitState_t)) != 0 || !st) {
        return NULL;
    }
    memset(st, 0, sizeof(AutoCodeJitState_t));
    if (initial_lib_path) {
        strncpy(st->active_path, initial_lib_path, MAX_LIB_NAME - 1);
        st->active_handle = dlopen(initial_lib_path, RTLD_NOW | RTLD_GLOBAL);
    }
    st->hot_swaps_performed = 0;
    st->version_generation = 1;
    st->is_hook_active = (st->active_handle != NULL);
    return st;
}

__attribute__((visibility("default")))
int autocode_jit_hot_swap(AutoCodeJitState_t* st, const char* new_lib_path) {
    if (!st || !new_lib_path) return -1;

    void* new_handle = dlopen(new_lib_path, RTLD_NOW | RTLD_GLOBAL);
    if (!new_handle) {
        return 1; /* [FEIL] Kunne ikke laste ny modul */
    }

    if (st->active_handle) {
        dlclose(st->active_handle);
    }

    st->active_handle = new_handle;
    strncpy(st->active_path, new_lib_path, MAX_LIB_NAME - 1);
    st->hot_swaps_performed++;
    st->version_generation++;
    st->is_hook_active = true;

    return 0; /* [✔ SUKSESS] Hot-swap gjennomført uten krasj */
}

__attribute__((visibility("default")))
void autocode_jit_free(AutoCodeJitState_t* st) {
    if (!st) return;
    if (st->active_handle) {
        dlclose(st->active_handle);
    }
    free(st);
}

#ifndef JSONRDR_APP_H
#define JSONRDR_APP_H
#include <stdint.h>

typedef enum {
    INIT,
    START,
    LOAD,
    RUN,
    EXIT
}app_state_e;

typedef struct {
    uint8_t is_full_screen;
    uint8_t status_code;
    app_state_e state;
    uint64_t file_size; 
    char* contents;
    char** lines_ptr;
    uint64_t total_lines;
    char* file_name;
    uint64_t* matches_ptr;
    uint64_t matches_count;
    uint64_t current_index;
} app_s ;

int init_app(app_s* app, float* progress);

#endif

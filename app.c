#include "app.h"
#include <stdio.h>
#include <stdlib.h>


int init_app(app_s* app, float* progress)
{
    FILE *read = fopen(app->file_name, "r");
    if (NULL == read) {
        printf("Could not open file!\n");
        return 1;
    }
    *progress = 25;
    printf("Analyzing file........\n");
    size_t num_read = fread(app->contents, sizeof(char), app->file_size, read);
    if (num_read != app->file_size) {
        printf("File could only be read partially!\n");
    }
    *progress = 50;
    // Making sure it is null terminated
    app->contents[app->file_size] = '\0';
    // File copied to memory can close now
    fclose(read);
    *progress = 60;
    for (uint64_t i = 0; i < app->file_size - 1; i++) {
        if (app->contents[i] == '}' && ((app->contents[i + 1] == ',') || (app->contents[i + 1] == '\n') || (app->contents[i + 1] == '\0'))) {
            app->total_lines++;
        }
    }
    if (app->total_lines <= 1) {
        app->total_lines = 1;
        app->lines_ptr = &app->contents;
    } else {
        app->lines_ptr = malloc((app->total_lines + 1) * sizeof(char *));
    if (NULL == app->lines_ptr) {
        printf("Not enough memory. Buy RAM!!!\n");
        return 1;
    }
    }
    
    *progress = 80;
    uint64_t string_index = 0;

    // First line of string
    app->lines_ptr[string_index++] = app->contents;

    for (uint64_t i = 1; i < app->file_size - 2; i++) {
        if (app->contents[i] == '}' && ((app->contents[i + 1] == ',') || (app->contents[i + 1] == '\n')) ) {
        app->contents[i + 1] = '\0';
        app->lines_ptr[string_index++] = app->contents + i + 2;
        }
    }

    printf("Total line: %lu\n",app->total_lines);
    printf("First line: %s\n",app->lines_ptr[0]);

    *progress = 100;
    return 0;
}
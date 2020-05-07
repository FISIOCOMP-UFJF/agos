//
// Created by sachetto on 06/05/2020.
//

#include "opts.h"
#include <string.h>
#include <stdio.h>

static const char *conversion_opt_string = "i:o:l:sh?";
static const struct option long_conversion_options[] = {{"input", required_argument, NULL, 'i'},
                                                        {"output", required_argument, NULL, 'o'},
                                                        {"single_cell_only", required_argument, NULL, 's'},
                                                        {"single_cell_language", required_argument, NULL, 'l'}};

char * get_file_from_path(const char * path) {
    char *last_slash = NULL;
    char *file = NULL;
    last_slash = strrchr(path, '/');

    if(last_slash) {
        file = strndup(last_slash + 1,  path - last_slash + 1);
        return file;
    }
    else {
        return strdup(path);
    }
}

char *get_filename_without_ext(const char *filename) {
    char *last_dot = NULL;
    char *file = NULL;
    last_dot = strrchr(filename, '.');

    if(!last_dot || last_dot == filename) return (char*)filename;

    file = strndup(filename,  strlen(filename) - strlen(last_dot));
    return file;
}

static void display_usage(char **argv) {
    printf("Usage: %s [options]\n\n", argv[0]);
    printf("Options:\n");
    printf("--input  | -i [input]. Input cellml file. Default NULL.\n");
    printf("--output | -o [output]. Output model name. Default: same as input.\n");
    printf("--single_cell_only | -s [true|false]. Generate only the single cell solver. Default: false.\n");
    printf("--single_cell_language | -l [c|python]. Language to generate the single cell model. Default: c.\n");
    printf("--help | -h. Shows this help and exit \n");
    exit(EXIT_FAILURE);
}

void parse_options(int argc, char **argv, struct options *options) {

    int opt = 0;
    int option_index;

    if(argc == 1) {
        display_usage(argv);
    }
    opt = getopt_long_only(argc, argv, conversion_opt_string, long_conversion_options, &option_index);

    options->input_file = NULL;
    options->output_name = NULL;
    options->single_cell_only = false;
    options->single_cell_lang = strdup("c");

    while(opt != -1) {
        switch(opt) {
            case 'i':
                options->input_file = strdup(optarg);
                break;
            case 'o':
                options->output_name = strdup(optarg);
                break;
            case 's':
                options->single_cell_only = true;
                break;
            case 'l':
                free(options->single_cell_lang);
                options->single_cell_lang = strdup(optarg);
                break;
            case 'h': /* fall-through is intentional */
            case '?':
                display_usage(argv);
                break;
            default:
                /* You won't actually get here. */
                break;
        }

        opt = getopt_long(argc, argv, conversion_opt_string, long_conversion_options, &option_index);
    }


    if(options->input_file == NULL) {
        display_usage(argv);
    }


}
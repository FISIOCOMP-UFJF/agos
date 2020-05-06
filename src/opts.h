//
// Created by sachetto on 06/05/2020.
//

#ifndef AGOS_OPTS_H
#define AGOS_OPTS_H

#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>


struct options {
    char *input_file;
    char *output_name;
    bool single_cell_only;
    char *single_cell_lang;
};


void parse_options(int argc, char **argv, struct options *options);
char * get_file_from_path(const char * path);
char *get_filename_without_ext(const char *filename);

#endif //AGOS_OPTS_H

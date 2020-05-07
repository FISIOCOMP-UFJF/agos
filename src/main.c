#include "compile.h"
#include "compile_python.h"
#include "opts.h"

int main(int argc, char **argv) {
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	struct options opts;

	parse_options(argc, argv, &opts);

    if(opts.output_name == NULL) {
        char *tmp = get_file_from_path(opts.input_file);
        opts.output_name = get_filename_without_ext(tmp);
        free(tmp);
    }
	
	LIBXML_TEST_VERSION
	doc = xmlReadFile (opts.input_file, NULL, 0);
	
	if (doc == NULL)
	{
		printf ("error: could not parse file %s\n", argv[1]);
		return (2);
	}	
	root_element = xmlDocGetRootElement (doc);

	// inicializando a variavel global
	pointer = root_element;

	if(princ() != SUCCESS){
		fprintf(stderr,"Unknow parsing error\n");
		return 1;
	}
	
	if(rewind_token_list(difvarlist) == NULL)
	{
		printf("ERROR - Differential Equation not found\n");
		exit(1);
	}

	// building param list and associating initial values
	initial_values(root_element);
	fix_duplicate_equations(root_element, alglist);

	sds modelname = sdsnew(opts.output_name);
	sds modelname2 = sdsnew(opts.output_name);
	sds modelname3 = sdsnew(opts.output_name);

    if(strcasecmp(opts.single_cell_lang, "c") == 0) {
        generate_c_solver(modelname);
    }
    else if(strcasecmp(opts.single_cell_lang, "python") == 0) {
        generate_python_solver(modelname);
    }
    else {
        fprintf(stderr, "[ERR] Language %s not supported!\n", opts.single_cell_lang);
        fprintf(stderr, "[ERR] Supported languages are: c or python!\n");
    }

    if(!opts.single_cell_only) {
        generate_cpu_model(modelname2);
        generate_gpu_model(modelname3);
    }

    sdsfree(modelname);
    sdsfree(modelname2);
    sdsfree(modelname3);

	return 0;
}

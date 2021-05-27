//
// Created by sachetto on 06/05/2020.
//

#ifndef AGOS_COMPILE_ODE_H
#define AGOS_COMPILE_ODE_H

#include "fsm.h"

void generate_ode_file(sds model_name);
int print_right_alg_ode(FILE *file, AlgList *list, TokenNode *orderedlist, struct if_else * all_ifs);
struct if_else * get_if_list_ode(IfList *list);

void print_inline_if_ode(FILE *file, sds *string, struct if_else current_if, char *name) {

    //TODO: fix get_if_list_ode to generate a list of ifs to allow us to call this function recursivelly
    if(file) {
        fprintf(file, "if(%s) {\n", current_if.if_condition);
        fprintf(file, "    %s = %s\n", name, current_if.if_statements);

        for (int i = 0; i < arrlen(current_if.else_if_statements); i++) {
            fprintf(file, "} elif(%s) {\n", current_if.else_if_conditions[i]);
            fprintf(file, "        %s = %s\n", name, current_if.else_if_statements[i]);
        }

        fprintf(file, "} else {\n");
        fprintf(file, "    %s = %s\n", name, current_if.else_statements);
        fprintf(file, "}\n");
    }

    if(string) {
        *string = sdscatprintf(*string, "if(%s){\n", current_if.if_condition);
        *string = sdscatprintf(*string, "    %s = %s\n", name, current_if.if_statements);

        for(int i = 0; i < arrlen(current_if.else_if_statements); i++) {
            *string = sdscatprintf(*string, "} elif (%s) {\n", current_if.else_if_conditions[i]);
            *string = sdscatprintf(*string, "    %s = %s\n", name, current_if.else_if_statements[i]);
        }

        *string = sdscatprintf(*string, "} else {\n");
        *string = sdscatprintf(*string, "    %s = %s", name, current_if.else_statements);
        *string = sdscatprintf(*string, "}\n");
    }

}

int print_eq_ode_file(FILE *file, sds *string, TokenNode *t, char *left_token_name, struct if_else *all_ifs) {
    if (file == NULL && string == NULL ) {
        printf("ERROR - Can't write in file or string, print_alg");
        exit(1);
    }

    if(string != NULL && *string == NULL) {
        printf("ERROR - Can't write in string, print_alg");
        exit(1);
    }

    TokenNode *cur = t;
    while (cur != NULL) {
        if (list_has_var(cur->token, algvarlist)) {
            if(file) {
                fprintf(file, "%s", cur->token.content);
            }
            if(string) {
                *string = sdscatprintf(*string,"%s", cur->token.content);
            }

        } else {
            if (cur->token.type == PI_W) {
                if(all_ifs) {
                    long if_index = strtol(cur->token.content, NULL, 10);

                    if (file) {
                        fprintf(file, "0.0;\n");
                    }
                    if (string) {
                        *string = sdscatprintf(*string, "0.0;\n");
                    }

                    print_inline_if_ode(file, string, all_ifs[if_index], left_token_name);

                }
            } else {
                if (!strcmp(cur->token.content, difflist->diffheader->freevar.content)) {
                    if(file) {
                        fprintf(file, "%s_new", cur->token.content);
                    }
                    if(string) {
                        *string = sdscatprintf(*string, "%s_new", cur->token.content);
                    }
                }
                else {
                    if (!cur->token.isdiff || cur->token.type == CPAR || cur->token.type == OPAR) {

                        if(cur->token.type==L_OP && strcmp(cur->token.tag, "and") == 0 && strcmp(cur->token.content, " and ") != 0) {
                            cur->token.content = strdup(" and ");
                        }

                        if(cur->token.tag && strcmp(cur->token.tag, "or") == 0 && strcmp(cur->token.content, " or ") != 0) {
                            cur->token.content = strdup(" or ");
                        }

                        if(file) {
                            fprintf(file, "%s", cur->token.content);
                        }

                        if(string) {
                            *string = sdscatprintf(*string, "%s", cur->token.content);
                        }
                    }
                    if (list_has_var(cur->token, difvarlist)) {
                        if (!cur->token.isdiff) {
                        }
                        else {
                            DiffList *curdiff = rewind_diff_list(difflist);
                            int found = 0;
                            while (curdiff != NULL && !found) {
                                if (!strcmp((const char *)curdiff->diffheader->diffvar.content, cur->token.content)) {
                                    found = 1;
                                }
                                else {
                                    curdiff = curdiff->next;
                                }
                            }
                            if (found) {
                                if(curdiff != NULL) {
                                    print_eq_ode_file(file, string, curdiff->diffheader->eq->next, NULL, all_ifs);
                                }
                            } else {
                                printf("ERROR - printfDiff Differential equation referenced, "
                                       "but not defined\n");
                                exit(1);
                            }
                        }
                    }
                }
            }
        }
        cur = cur->next;
    }

    return 0;
}


int print_diff_ode_file(FILE *file, DiffList *list, struct if_else *all_ifs) {
    if (file == NULL) {
        printf("ERROR - Can't write in file, print_diff");
        exit(1);
    }
    DiffList *curl = rewind_diff_list(list);
    TokenNode *cur = NULL;
    char tmp[2048];
    while (curl != NULL) {
        fprintf(file, "\node %s' = ", curl->diffheader->diffvar.content);
        sprintf(tmp, "%s'", curl->diffheader->diffvar.content);
        cur = curl->diffheader->eq->next;
        print_eq_ode_file(file, NULL, cur, tmp, all_ifs);
        curl = curl->next;
    }
    return 0;
}

//TODO: do the same with C code to avoid the ifdefs
struct if_else * get_if_list_ode(IfList *list) {

    struct if_else *ifs_list = NULL;

    IfList *curl = rewind_if_list(list);
    TokenNode *cur = NULL;
    int i = 0;
    while (curl != NULL) {
        struct if_else current;
        current.if_condition = sdsempty();
        current.if_statements = sdsempty();
        current.else_statements = sdsempty();
        current.else_if_statements = NULL;
        current.else_if_conditions = NULL;

        // printing the condition
        cur = curl->ifheader->cond;
        print_eq_ode_file(NULL, &current.if_condition, cur, NULL, NULL); // printing if condition

        cur = curl->ifheader->piece;

        print_eq_ode_file(NULL, &current.if_statements, cur, NULL, NULL); // printing the return of if

        while (curl->next != NULL &&
               curl->next->ifheader->if_counter == curl->ifheader->if_counter) {
            curl = curl->next;

            sds else_if_condition = sdsempty();
            sds else_if_statement = sdsempty();

            cur = curl->ifheader->cond;
            print_eq_ode_file(NULL, &else_if_condition, cur, NULL, NULL); // printing if condition
            cur = curl->ifheader->piece;
            print_eq_ode_file(NULL, &else_if_statement, cur, NULL, NULL); // printing the return of if
            arrpush(current.else_if_conditions, else_if_condition);
            arrpush(current.else_if_statements, else_if_statement);
        }

        cur = curl->ifheader->other;
        print_eq_ode_file(NULL, &current.else_statements, cur, NULL, NULL); // printing the return of else

        arrpush(ifs_list, current);

        i++;
        curl = curl->next;
    }

    return ifs_list;

}

int print_right_alg_ode_file(FILE *file, AlgList *list, TokenNode *orderedlist, struct if_else * all_ifs) {
    if (file == NULL) {
        printf("ERROR - Can't write in file, print_alg");
        exit(1);
    }
    TokenNode *curl = rewind_token_list(orderedlist);
    TokenNode *cur = NULL;
    AlgList *curalg = NULL;

    while (curl != NULL) {
        curalg = rewind_alg_list(list);
        while (strcmp(curalg->eq->token.content, curl->token.content) != 0) {
            curalg = curalg->next;
        }

	   	fprintf(file, "\n%s = ", curalg->eq->token.content);
        cur = curalg->eq;
        cur = cur->next->next;
        sds tmp = sdsnew("");
        tmp = sdscatfmt(tmp, "%s", curalg->eq->token.content);

        print_eq_ode_file(file, NULL, cur, tmp, all_ifs);

        curl = curl->next;
    }
    fprintf(file, "\n");
    return 0;
}

#if 0
int print_right_alg_ode_file(FILE *file, AlgList *list, TokenNode *orderedlist) {
    if (file == NULL) {
        printf("ERROR - Can't write in file, print_alg");
        exit(1);
    }
    TokenNode *curl = rewind_token_list(orderedlist);
    TokenNode *cur = NULL;
    AlgList *curalg = NULL;

    while (curl != NULL) {
        curalg = rewind_alg_list(list);
        while (strcmp(curalg->eq->token.content, curl->token.content) != 0) {
            curalg = curalg->next;
        }

        fprintf(file, "\n%s = ", curalg->eq->token.content);
        cur = curalg->eq;
        cur = cur->next->next;
        sds tmp = sdsnew("");
        tmp = sdscatfmt(tmp, "%s", curalg->eq->token.content);

        print_eq_ode_file(file, cur, tmp);

        fprintf(file, ";");
        curl = curl->next;
    }
    fprintf(file, "\n");
    return 0;
}
#endif

void generate_ode_file(sds model_name) {

    preced_alg_list = create_preced_alg_list(rewind_alg_list(alglist));
    AlgList *cural;
    TokenNode *resolved_dep_list = NULL;

   	struct if_else *all_ifs = get_if_list_ode(iflist);

    while (preced_alg_list != NULL) {
        cural = rewind_alg_list(preced_alg_list);
        while (cural != NULL) {
            cural = cural->next;
        }
        cural = rewind_alg_list(preced_alg_list);

        while (cural != NULL) {

            TokenNode *cureq = cural->eq->next;
            if (cureq == NULL) {
                resolved_dep_list = add_list(cural->eq->token, resolved_dep_list);
                preced_alg_list = delete_from_list(preced_alg_list, cural->eq->token);
                cural = cural->next;
            } else {

                while (cureq != NULL) {
                    if (list_has_var(cureq->token, resolved_dep_list)) {
                        if (cureq->prev != NULL) {
                            cureq->prev->next = cureq->next;
                        }
                        if (cureq->next != NULL) {
                            cureq->next->prev = cureq->prev;
                        }
                    }
                    cureq = cureq->next;
                }
                cural = cural->next;
            }
        }
    }

    FILE *file;
    sds filename = sdsdup(model_name);
    filename = sdscat(filename, ".ode");

    file = fopen(filename, "w");

    TokenNode *cur = rewind_token_list(difvarlist);

    fprintf(file, "#Parameters\n");
    cur = rewind_token_list(parvarlist);
    while (cur != NULL) {
		if(strcmp(cur->token.content, "time") == 0) {
        	fprintf(file, "time_new = time;\n");
		}
		else {
        	fprintf(file, "%s = %.15ef;\n", cur->token.content, cur->initialvalue);
		}
        cur = cur->next;
    }

    print_right_alg_ode_file(file, alglist, resolved_dep_list, all_ifs);

    print_diff_ode_file(file, difflist, all_ifs);

    cur = rewind_token_list(difvarlist);

    fprintf(file, "\n\n#initial conditions");

    while (cur != NULL) {
        fprintf(file, "\ninitial %s = %e", cur->token.content, cur->initialvalue);
        cur = cur->next;
    }


    fclose(file);

    sdsfree(filename);
}

#endif //AGOS_COMPILE_ODE_H

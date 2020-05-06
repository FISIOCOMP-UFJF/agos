#ifndef _COMPILE_H_
#define _COMPILE_H_
#include "fsm.h"
#include <string.h>
#include "string/sds.h"

int print_diff(FILE *file, DiffList *list);
int print_right_alg(FILE *file, AlgList *list, TokenNode *orderedlist);
int print_eq(FILE *file, TokenNode *t, char *left_token_name);
int print_if(FILE *file, IfList *list);
void initial_values(xmlNode *root);
int set_initial_value(char *name, double initvalue, char *units);

// Fix duplicated algebraic variables
void fix_duplicate_equations(xmlNode *root, AlgList *alg);
void fix(xmlNode *root, AlgList *list);
void change_duplicated_equations(int eqnumber, int bottom, int top, char *cname);
void fix_node(TokenNode *node, char *oldname, char *newname);

int print_if(FILE *file, IfList *list) {
    if (file == NULL) {
        printf("ERROR - Can't write in file, print_alg");
        exit(1);
    }
    IfList *curl = rewind_if_list(list);
    TokenNode *cur = NULL;
    int i = 0;
    while (curl != NULL) {
        fprintf(file, "\n#define IFNUMBER_%d(name)", curl->ifheader->if_counter);
        fprintf(file, "if(");
        // printing the condition
        cur = curl->ifheader->cond;
        print_eq(file, cur, NULL); // printing if condition
        fprintf(file, ") { (name) = ");
        cur = curl->ifheader->piece;
        print_eq(file, cur, NULL); // printing the return of if
        fprintf(file, ";    } ");

        while (curl->next != NULL &&
               curl->next->ifheader->if_counter == curl->ifheader->if_counter) {
            curl = curl->next;
            fprintf(file, " else if(");

            cur = curl->ifheader->cond;
            print_eq(file, cur, NULL); // printing if condition
            fprintf(file, "){ (name) = ");
            cur = curl->ifheader->piece;
            print_eq(file, cur, NULL); // printing the return of if
            fprintf(file, ";");
            fprintf(file, "    }");
        }

        fprintf(file, " else{");
        fprintf(file, " (name) = ");
        cur = curl->ifheader->other;
        print_eq(file, cur, NULL); // printing the return of else
        fprintf(file, ";");
        fprintf(file, "    }");

        i++;
        curl = curl->next;
    }
    return 0;
}
int print_diff(FILE *file, DiffList *list) {
    if (file == NULL) {
        printf("ERROR - Can't write in file, print_diff");
        exit(1);
    }
    DiffList *curl = rewind_diff_list(list);
    TokenNode *cur = NULL;
    int count = 0;
    char tmp[2048];
    while (curl != NULL) {
        fprintf(file, "\n    rDY[%d] = ", count);
        sprintf(tmp, "rDY[%d]", count);
        cur = curl->diffheader->eq->next;
        print_eq(file, cur, tmp);
        fprintf(file, ";");
        curl = curl->next;
        count++;
    }
    return 0;
}

int print_diff_cvode(FILE *file, DiffList *list) {
    if (file == NULL) {
        printf("ERROR - Can't write in file, print_diff");
        exit(1);
    }
    DiffList *curl = rewind_diff_list(list);
    TokenNode *cur = NULL;
    int count = 0;
    char tmp[2048];
    while (curl != NULL) {
        fprintf(file, "\n    NV_Ith_S(rDY, %d) = ", count);
        sprintf(tmp, "NV_Ith_S(rDY, %d)", count);
        cur = curl->diffheader->eq->next;
        print_eq(file, cur, tmp);
        fprintf(file, ";");
        curl = curl->next;
        count++;
    }
    return 0;
}

int print_eq(FILE *file, TokenNode *t, char *left_token_name) {
    if (file == NULL) {
        printf("ERROR - Can't write in file, print_alg");
        exit(1);
    }
    TokenNode *cur = t;
    while (cur != NULL) {
        if (list_has_var(cur->token, algvarlist)) {
            fprintf(file, "calc_");
            fprintf(file, "%s", cur->token.content);
            // 08/03/2008    fprintf(file, "()");
        } else {
            if (cur->token.type == PI_W) {
                fprintf(file, "0.0f;\n");

                fprintf(file, "    IFNUMBER_%s(%s)", cur->token.content, left_token_name);
            } else {
                if (!strcmp(cur->token.content, difflist->diffheader->freevar.content))
                    fprintf(file, "%s_new", cur->token.content);
                else {
                    if (!cur->token.isdiff || cur->token.type == CPAR ||
                        cur->token.type == OPAR)
                        fprintf(file, "%s", cur->token.content);
                    if (list_has_var(cur->token, difvarlist)) {
                        if (!cur->token.isdiff)
                            fprintf(file, "_old_");
                        else {
                            DiffList *curdiff = rewind_diff_list(difflist);
                            int found = 0;
                            while (curdiff != NULL && !found) {
                                if (!strcmp((const char *)curdiff->diffheader->diffvar.content,
                                            cur->token.content))
                                    found = 1;
                                else
                                    curdiff = curdiff->next;
                            }
                            if (found) {
                                if(curdiff != NULL) {
                                    print_eq(file, curdiff->diffheader->eq->next, NULL);
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

void initial_values(xmlNode *root) {
    parvarlist = create_param_list(varlist, difvarlist, algvarlist);
    xmlNode *curmodel = root;
    xmlNode *curvar;

    while (curmodel != NULL) {
        // if the file is just a MathML not a CellML
        if (!strcmp((char *)curmodel, "math"))
            break;
        curmodel = get_component(curmodel);
        if (curmodel != NULL) {
            curvar = get_variable(curmodel);
            while (curvar != NULL) {
                char *name = "";
                double initvalue = 0.0;
                char * units = "";
                // looking for initial_values in the variable
                xmlAttr *attr = curvar->properties;
                while (attr != NULL) {
                    if (!strcmp("initial_value", (char *)attr->name))
                        break;
                    attr = attr->next;
                }
                // if a initial value was found
                if (attr != NULL) {
                    initvalue = atof((char *)xmlNodeGetContent((xmlNode *)attr));
                }
                // take the variable name
                attr = curvar->properties;
                while (attr != NULL) {
                    if (!strcmp("name", (char *)attr->name))
                        break;
                    attr = attr->next;
                }
                if (attr != NULL) {
                    name = (char *)xmlNodeGetContent((xmlNode *)attr);
                }
                attr = curvar->properties;
                while (attr != NULL) {
                    if (!strcmp("units", (char *)attr->name))
                        break;
                    attr = attr->next;
                }
                if (attr != NULL) {
                    units = (char *)xmlNodeGetContent((xmlNode *)attr);
                }
                set_initial_value(name, initvalue, units);

                curvar = get_variable(curvar);
            }
        }
    }
}

int set_initial_value(char *name, double initvalue, char *units) {
    // creating the param list

    TokenNode *curdif = rewind_token_list(difvarlist);

    int found = 0;
    // searching in difflist
    while ( (curdif != NULL)  && (!found)) {
        if (!strcmp(strNoSpace(name), strNoSpace(curdif->token.content))) {
            curdif->units = units;
            if (initvalue != 0.0) {
                curdif->initialvalue = initvalue;
            }
            found = 1;
        }

        curdif = curdif->next;
    }
    TokenNode *curalg = rewind_token_list(algvarlist);
    // searching in alglist
    while ( (curalg != NULL) && (!found) ) {
        if (!strcmp(name, strNoSpace(curalg->token.content))) {
            curalg->units = units;
            if (initvalue != 0.0)
                curalg->initialvalue = initvalue;
            found = 1;
        }

        curalg = curalg->next;
    }
    TokenNode *curpar = rewind_token_list(parvarlist);
    // searching in parlist
    while ((curpar != NULL) && (!found)) {
        if (!strcmp(name, strNoSpace(curpar->token.content))) {
            curpar->units = units;
            if (initvalue != 0.0)
                curpar->initialvalue = initvalue;
            found = 1;
        }
        curpar = curpar->next;
    }
    if (!found)
        return 0;
    else
        return 1;
}

void fix_duplicate_equations(xmlNode *root, AlgList *alg) {
    xmlNode *curroot = root;
    TokenNode *list = NULL;
    AlgList *dpllist = NULL;
    AlgList *curalg = rewind_alg_list(alg);

    // creating a list of duplicated equations
    while (curalg != NULL) {
        Token t = curalg->eq->token;
        if (!list_has_var(t, list))
            list = attach_token(t, list);
        else {
            dpllist = attach_alg_eq(NULL, dpllist); // PODE DAR PAU AKI
            dpllist->number = curalg->number;
        }
        curalg = curalg->next;
    }

    dpllist = rewind_alg_list(dpllist);
    fix(curroot, dpllist);
}

void fix(xmlNode *root, AlgList *list) {
    xmlNode *curroot = root;
    AlgList *dpllist = list;
    int counter = -1; // to count the equations
    // searching for the duplicated equation
    while ((curroot = get_component(curroot)) != NULL) {
        xmlNode *math = get_math(curroot);
        xmlNode *apply = NULL;
        if (math != NULL)
            apply = math->children;
        int component = counter;
        while (apply != NULL) {
            if (!strcmp((char *)apply->name, "apply"))
                counter++;
            apply = apply->next;
        }
        // getting the component name*/
        xmlAttr *name = curroot->properties;
        char *cname = NULL;
        while (name != NULL) {
            if (!strcmp((char *)name->name, "name"))
                break;
            name = name->next;
        }
        if (name != NULL) {
            // printf("--------%s\n",name->children->content);
            cname = (char *)xmlNodeGetContent((xmlNode *)name);
        }
        if (dpllist != NULL)

            while (counter >= dpllist->number && component < dpllist->number) {
                printf("A duplicated algebraic equation was found at component "
                               "%s\nTrying to fix the problem\nPlease verify if the equations "
                               "generated are correct\n",
                       cname);

                change_duplicated_equations(dpllist->number, component + 1, counter, cname);
                dpllist = dpllist->next;
                if (dpllist == NULL)
                    break;
            }
    }
}

void change_duplicated_equations(int eqnumber, int bottom, int top, char *cname) {
    AlgList *alg = rewind_alg_list(alglist);
    DiffList *diff = rewind_diff_list(difflist);
    char *oldname = NULL;

    while (alg != NULL) {
        if (alg->number == eqnumber)
            break;
        alg = alg->next;
    }
    if (alg != NULL)
        oldname = alg->eq->token.content;

    char *newname =
            (char *)calloc(strlen(oldname) + strlen(cname) + 13, sizeof(char));
    sprintf(newname, "%s_duplicated_%s", oldname, cname);

    Token t;
    t.content = newname;
    algvarlist = attach_token(t, algvarlist);
    /*
    xmlNode *var = comp;
    while((var = get_variable(var)) != NULL)
    {
        xmlAttr *attr = var->properties;
        while(attr != NULL)
        {
            if(!strcmp((char*)attr->name, "name"))
                break;
            attr = attr->next;
        }
        if(attr != NULL)
        {
            attr->children->content = (xmlChar*)newname;
            break;
        }
    }
    */
    alg = rewind_alg_list(alglist);

    while (alg != NULL) {
        if (alg->number >= bottom && alg->number <= top) {
            TokenNode *cur = alg->eq;
            fix_node(cur, oldname, newname);
        }
        if (alg->number > top)
            break;
        alg = alg->next;
    }

    while (diff != NULL) {
        if (diff->number >= bottom && diff->number <= top) {
            TokenNode *cur = diff->diffheader->eq;
            fix_node(cur, oldname, newname);
        }
        if (diff->number > top)
            break;
        diff = diff->next;
    }
}

void fix_node(TokenNode *node, char *oldname, char *newname) {
    TokenNode *cur = node;
    while (cur != NULL) {
        if (cur->token.type == PI_W) {
            long num = strtol(cur->token.content, NULL, 10);
            IfList *ilist = rewind_if_list(iflist);
            int i;

            for (i = 0; i < num; i++) {
                ilist = ilist->next;
            }

            fix_node(ilist->ifheader->cond, oldname, newname);
            fix_node(ilist->ifheader->piece, oldname, newname);
            fix_node(ilist->ifheader->other, oldname, newname);
        }

        if (!strcmp(cur->token.content, oldname)) {
            cur->token.content = newname;
        }
        cur = cur->next;

    }
}

void generate_cpu_model(sds model_name) {

    if (eq_counter <= 0) {
        printf("ERROR - Equation not found\n");
        exit(1);
    }

    preced_alg_list = create_preced_alg_list(rewind_alg_list(alglist));
    AlgList *cural = NULL;
    TokenNode *resolved_dep_list = NULL;

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
    filename = sdscat(filename, ".c");

    sds headername = sdsdup(model_name);
    headername = sdscat(headername, ".h");

    file = fopen(filename, "w");
    fprintf(file, "#include \"%s\"\n", headername);
    fprintf(file, "#include <stdlib.h>\n");

    fprintf(file, "real max_step;\n"
                  "real min_step;\n"
                  "real abstol;\n"
                  "real reltol;\n"
                  "bool adpt;\n"
                  "real *ode_dt, *ode_previous_dt, *ode_time_new;\n\n");

    fprintf(file, "GET_CELL_MODEL_DATA(init_cell_model_data) {\n"
            "\n"
            "    if(get_initial_v)\n"
            "        cell_model->initial_v = INITIAL_V;\n"
            "    if(get_neq)\n"
            "        cell_model->number_of_ode_equations = NEQ;\n"
            "}\n"
            "\n");


    // SET INITIAL CONDITIONS CPU
    fprintf(
            file,
            "SET_ODE_INITIAL_CONDITIONS_CPU(set_model_initial_conditions_cpu) {\n"
            "\n"
            "    log_to_stdout_and_file(\"Using %s CPU model\\n\");\n"
            "\n"
            "    uint32_t num_cells = solver->original_num_cells;\n"
            "    solver->sv = (real*)malloc(NEQ*num_cells*sizeof(real));\n"
            "\n"
            "    max_step = solver->max_dt;\n"
            "    min_step = solver->min_dt;\n"
            "    abstol   = solver->abs_tol;\n"
            "    reltol   = solver->rel_tol;\n"
            "    adpt     = solver->adaptive;\n"
            "\n"
            "    if(adpt) {\n"
            "        ode_dt = (real*)malloc(num_cells*sizeof(real));\n"
            "\n"
                "        OMP(parallel for)\n"
            "        for(int i = 0; i < num_cells; i++) {\n"
            "            ode_dt[i] = solver->min_dt;\n"
            "        }\n"
            "\n"
            "        ode_previous_dt = (real*)calloc(num_cells, sizeof(real));\n"
            "        ode_time_new    = (real*)calloc(num_cells, sizeof(real));\n"
            "        log_to_stdout_and_file(\"Using Adaptive Euler model to solve the ODEs\\n\");\n"
            "    } else {\n"
            "        log_to_stdout_and_file(\"Using Euler model to solve the ODEs\\n\");\n"
            "    }\n"
            "\n"
            "\n"
            "    OMP(parallel for)\n"
            "    for(uint32_t i = 0; i < num_cells; i++) {\n\n"
            "        real *sv = &solver->sv[i * NEQ];", model_name);

    fprintf(file, "\n\n");
    TokenNode *cur = rewind_token_list(difvarlist);
    int counter = 0;
    while (cur != NULL) {
        fprintf(file, "        sv[%d] = %ef; //%s %s \n", counter, cur->initialvalue,
                cur->token.content, cur->units);
        cur = cur->next;
        counter++;
    }
    fprintf(file, "    }\n}\n\n");

    // SOLVE_MODEL_ODES
    fprintf(file, "SOLVE_MODEL_ODES(solve_model_odes_cpu) {\n"
                  "\n"
                  "    uint32_t sv_id;\n"
                  "\n"
                  "    size_t num_cells_to_solve = ode_solver->num_cells_to_solve;\n"
                  "    uint32_t * cells_to_solve = ode_solver->cells_to_solve;\n"
                  "    real *sv = ode_solver->sv;\n"
                  "    real dt = ode_solver->min_dt;\n"
                  "    uint32_t num_steps = ode_solver->num_steps;\n"
                  "\n"
                  "    #pragma omp parallel for private(sv_id)\n"
                  "    for (u_int32_t i = 0; i < num_cells_to_solve; i++) {\n"
                  "\n"
                  "        if(cells_to_solve)\n"
                  "            sv_id = cells_to_solve[i];\n"
                  "        else\n"
                  "            sv_id = i;\n"
                  "\n"
                  "        if(adpt) {\n"
                  "\n"
                  "            solve_forward_euler_cpu_adpt(sv + (sv_id * NEQ), current_t + dt, stim_currents[i], sv_id);\n"
                  "        }\n"
                  "        else {\n"
                  "            for (int j = 0; j < num_steps; ++j) {\n"
                  "                solve_model_ode_cpu(dt, sv + (sv_id * NEQ), stim_currents[i]);\n"
                  "            }\n"
                  "\n"
                  "        }\n"
                  "\n"
                  "    }\n"
                  "}\n\n");

    // SOLVE MODEL ODE CPU
    fprintf(file,"void solve_model_ode_cpu(real dt, real *sv, real stim_current)  {\n"
                 "\n"
                 "    real rY[NEQ], rDY[NEQ];\n"
                 "\n"
                 "    for(int i = 0; i < NEQ; i++)\n"
                 "        rY[i] = sv[i];\n"
                 "\n"
                 "    RHS_cpu(rY, rDY, stim_current, dt);\n"
                 "\n"
                 "    for(int i = 0; i < NEQ; i++)\n"
                 "        sv[i] = dt*rDY[i] + rY[i];\n"
                 "}\n\n");


    fprintf(file, "\n\nfloat __agos_factorial(int f){\n\tif(f>=0 && "
            "f<2)\n\t\treturn 1.0;\n\telse if(f < 0)\n\t\treturn "
            "0.0/0.0;\n\tfor(int i=f-1; i>=2; i--)\n\t\tf *= i;\n\treturn "
            "(float)f;\n}\n\n");

    fprintf(file, "void solve_forward_euler_cpu_adpt(real *sv, real stim_curr, real final_time, int sv_id) {\n"
                  "\n"
                  "    const real _beta_safety_ = 0.8;\n"
                  "    int numEDO = NEQ;\n"
                  "\n"
                  "    real rDY[numEDO];\n"
                  "\n"
                  "    real _tolerances_[numEDO];\n"
                  "    real _aux_tol = 0.0;\n"
                  "    //initializes the variables\n"
                  "    ode_previous_dt[sv_id] = ode_dt[sv_id];\n"
                  "\n"
                  "    real edos_old_aux_[numEDO];\n"
                  "    real edos_new_euler_[numEDO];\n"
                  "    real *_k1__ = (real*) malloc(sizeof(real)*numEDO);\n"
                  "    real *_k2__ = (real*) malloc(sizeof(real)*numEDO);\n"
                  "    real *_k_aux__;\n"
                  "\n"
                  "    real *dt = &ode_dt[sv_id];\n"
                  "    real *time_new = &ode_time_new[sv_id];\n"
                  "    real *previous_dt = &ode_previous_dt[sv_id];\n"
                  "\n"
                  "    if(*time_new + *dt > final_time) {\n"
                  "       *dt = final_time - *time_new;\n"
                  "    }\n"
                  "\n"
                  "    RHS_cpu(sv, rDY, stim_curr, *dt);\n"
                  "    *time_new += *dt;\n"
                  "\n"
                  "    for(int i = 0; i < numEDO; i++){\n"
                  "        _k1__[i] = rDY[i];\n"
                  "    }\n"
                  "\n"
                  "    const double __tiny_ = pow(abstol, 2.0);\n"
                  "\n"
                  "    int count = 0;\n"
                  "\n"
                  "    int count_limit = (final_time - *time_new)/min_step;\n"
                  "\n"
                  "    int aux_count_limit = count_limit+2000000;\n"
                  "\n"
                  "    if(aux_count_limit > 0) {\n"
                  "        count_limit = aux_count_limit;\n"
                  "    }\n"
                  "\n"
                  "    while(1) {\n"
                  "\n"
                  "        for(int i = 0; i < numEDO; i++) {\n"
                  "            //stores the old variables in a vector\n"
                  "            edos_old_aux_[i] = sv[i];\n"
                  "            //computes euler method\n"
                  "            edos_new_euler_[i] = _k1__[i] * *dt + edos_old_aux_[i];\n"
                  "            //steps ahead to compute the rk2 method\n"
                  "            sv[i] = edos_new_euler_[i];\n"
                  "        }\n"
                  "\n"
                  "        *time_new += *dt;\n"
                  "        RHS_cpu(sv, rDY, stim_curr, *dt);\n"
                  "        *time_new -= *dt;//step back\n"
                  "\n"
                  "        double greatestError = 0.0, auxError = 0.0;\n"
                  "        for(int i = 0; i < numEDO; i++) {\n"
                  "            //stores the new evaluation\n"
                  "            _k2__[i] = rDY[i];\n"
                  "            _aux_tol = fabs(edos_new_euler_[i])*reltol;\n"
                  "            _tolerances_[i] = (abstol > _aux_tol )?abstol:_aux_tol;\n"
                  "            //finds the greatest error between  the steps\n"
                  "            auxError = fabs(( (*dt/2.0)*(_k1__[i] - _k2__[i])) / _tolerances_[i]);\n"
                  "\n"
                  "            greatestError = (auxError > greatestError) ? auxError : greatestError;\n"
                  "        }\n"
                  "        ///adapt the time step\n"
                  "        greatestError += __tiny_;\n"
                  "        *previous_dt = *dt;\n"
                  "        ///adapt the time step\n"
                  "        *dt = _beta_safety_ * (*dt) * sqrt(1.0f/greatestError);\n"
                  "\n"
                  "        if (*time_new + *dt > final_time) {\n"
                  "            *dt = final_time - *time_new;\n"
                  "        }\n"
                  "\n"
                  "        //it doesn't accept the solution\n"
                  "        if ( count < count_limit  && (greatestError >= 1.0f)) {\n"
                  "            //restore the old values to do it again\n"
                  "            for(int i = 0;  i < numEDO; i++) {\n"
                  "                sv[i] = edos_old_aux_[i];\n"
                  "            }\n"
                  "\n"
                  "            count++;\n"
                  "            //throw the results away and compute again\n"
                  "        } else{//it accepts the solutions\n"
                  "\n"
                  "\n"
                  "            if(greatestError >=1.0) {\n"
                  "                printf(\"Accepting solution with error > %%lf \\n\", greatestError);\n"
                  "            }\n"
                  "\n"
                  "            if (*dt < min_step) {\n"
                  "                *dt = min_step;\n"
                  "            }\n"
                  "\n"
                  "            else if (*dt > max_step && max_step != 0) {\n"
                  "                *dt = max_step;\n"
                  "            }\n"
                  "\n"
                  "            if (*time_new + *dt > final_time) {\n"
                  "                *dt = final_time - *time_new;\n"
                  "            }\n"
                  "\n"
                  "            _k_aux__ = _k2__;\n"
                  "            _k2__\t= _k1__;\n"
                  "            _k1__\t= _k_aux__;\n"
                  "\n"
                  "            //it steps the method ahead, with euler solution\n"
                  "            for(int i = 0; i < numEDO; i++){\n"
                  "                sv[i] = edos_new_euler_[i];\n"
                  "            }\n"
                  "\n"
                  "            if(*time_new + *previous_dt >= final_time){\n"
                  "                if((fabs(final_time - *time_new) < 1.0e-5) ){\n"
                  "                    break;\n"
                  "                }else if(*time_new < final_time){\n"
                  "                    *dt = *previous_dt = final_time - *time_new;\n"
                  "                    *time_new += *previous_dt;\n"
                  "                    break;\n"
                  "\n"
                  "                }else{\n"
                  "                    printf(\"Error: time_new %%.20lf final_time %%.20lf diff %%e \\n\", *time_new , final_time, fabs(final_time - *time_new) );\n"
                  "                    break;\n"
                  "                }\n"
                  "            }else{\n"
                  "                *time_new += *previous_dt;\n"
                  "            }\n"
                  "\n"
                  "        }\n"
                  "    }\n"
                  "\n"
                  "    free(_k1__);\n"
                  "    free(_k2__);\n"
                  "}\n\n");

    // RHS CPU
    fprintf(file,
            "void RHS_cpu(const real *sv, real *rDY_, real stim_current, real dt) {\n\n");
    fprintf(file, "    //State variables\n");

    cur = rewind_token_list(difvarlist);
    counter = 0;
    while (cur != NULL) {
        fprintf(file, "    const real %s_old_ = sv[%d];\n", cur->token.content,
                counter);
        cur = cur->next;
        counter++;
    }

    fprintf(file, "\n");

    fprintf(file, "    #include \"%s_common.inc.c\"", model_name);

    fprintf(file, "\n\n}\n\n");

    fclose(file);

    sds common_name = sdsdup(model_name);
    common_name = sdscat(common_name, "_common.inc.c");

    file = fopen(common_name, "w");

    print_if(file, iflist);

    fprintf(file, "    //Parameters\n");
    cur = rewind_token_list(parvarlist);
    while (cur != NULL) {
        fprintf(file, "    const real %s = %.15ef;\n", cur->token.content,
                cur->initialvalue);
        cur = cur->next;
    }

    print_right_alg(file, alglist, resolved_dep_list);

    print_diff(file, difflist);

    fclose(file);

    // HEADER FILE
    file = fopen(headername, "w");

	sds model_upper = sdsdup(model_name);
	sdstoupper(model_upper);

    sdstoupper(model_name);
    fprintf(file, "#ifndef MONOALG3D_MODEL_%s_H\n", model_upper);
    fprintf(file, "#define MONOALG3D_MODEL_%s_H\n\n", model_upper);

    fprintf(file, "#include \"model_common.h\"\n"
                  "#include \"model_gpu_utils.h\"\n\n");

    cur = rewind_token_list(difvarlist);

    fprintf(file, "#define NEQ %d\n", counter);
    fprintf(file, "#define INITIAL_V (%lff)\n\n", cur->initialvalue);

    fprintf(file,
            "#ifdef __CUDACC__\n"
            "\n"
            "__global__ void kernel_set_model_initial_conditions(real *sv, int num_volumes);\n"
            "\n"
            "__global__ void solve_gpu(real cur_time, real dt, real *sv, real* stim_currents,\n"
            "                          uint32_t *cells_to_solve, uint32_t num_cells_to_solve,\n"
            "                          int num_steps);\n"
            "\n"
            "inline __device__ void RHS_gpu(real *sv, real *rDY, real stim_current, int thread_id, real dt);\n"
            "inline __device__ void solve_forward_euler_gpu_adpt(real *sv, real stim_curr, real final_time, int thread_id);\n"
            "\n"
            "#endif\n"
            "\n"
            "void RHS_cpu(const real *sv, real *rDY_, real stim_current, real dt);\n"
            "inline void solve_forward_euler_cpu_adpt(real *sv, real stim_curr, real final_time, int thread_id);\n"
            "\n"
            "void solve_model_ode_cpu(real dt, real *sv, real stim_current);\n\n");

    fprintf(file, "#endif //MONOALG3D_MODEL_%s_H\n\n", model_upper);

    fclose(file);
	sdsfree(model_upper);
    sdsfree(headername);
    sdsfree(filename);
}

void generate_gpu_model(sds model_name) {

    if (eq_counter <= 0) {
        printf("ERROR - Equation not found\n");
        exit(1);
    }

    preced_alg_list = create_preced_alg_list(rewind_alg_list(alglist));
    AlgList *cural;
    TokenNode *resolved_dep_list = NULL;
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
    filename = sdscat(filename, ".cu");

    sds headername = sdsdup(model_name);
    headername = sdscat(headername, ".h");

    file = fopen(filename, "w");
    fprintf(file, "#include \"%s\"\n", headername);

    fprintf(file, "#include <stddef.h>\n"
                  "#include <stdint.h>\n"
                  "\n"
                  "__constant__  size_t pitch;\n"
                  "__constant__  real abstol;\n"
                  "__constant__  real reltol;\n"
                  "__constant__  real max_dt;\n"
                  "__constant__  real min_dt;\n"
                  "__constant__  uint8_t use_adpt;"
                  "size_t pitch_h;"
                  "\n\n");

    // SET_ODE_INITIAL_CONDITIONS_GPU
    fprintf(file, "extern \"C\" SET_ODE_INITIAL_CONDITIONS_GPU(set_model_initial_conditions_gpu) {\n"
                  "\n"
                  "    uint8_t use_adpt_h = (uint8_t)solver->adaptive;\n"
                  "\n"
                  "    check_cuda_error(cudaMemcpyToSymbol(use_adpt, &use_adpt_h, sizeof(uint8_t)));\n"
                  "    log_to_stdout_and_file(\"Using %s GPU model\\n\");\n"
                  "\n"
                  "    uint32_t num_volumes = solver->original_num_cells;\n"
                  "\n"
                  "    if(use_adpt_h) {\n"
                  "        real reltol_h = solver->rel_tol;\n"
                  "        real abstol_h = solver->abs_tol;\n"
                  "        real max_dt_h = solver->max_dt;\n"
                  "        real min_dt_h = solver->min_dt;\n"
                  "\n"
                  "        check_cuda_error(cudaMemcpyToSymbol(reltol, &reltol_h, sizeof(real)));\n"
                  "        check_cuda_error(cudaMemcpyToSymbol(abstol, &abstol_h, sizeof(real)));\n"
                  "        check_cuda_error(cudaMemcpyToSymbol(max_dt, &max_dt_h, sizeof(real)));\n"
                  "        check_cuda_error(cudaMemcpyToSymbol(min_dt, &min_dt_h, sizeof(real)));\n"
                  "        log_to_stdout_and_file(\"Using Adaptive Euler model to solve the ODEs\\n\");\n"
                  "    } else {\n"
                  "        log_to_stdout_and_file(\"Using Euler model to solve the ODEs\\n\");\n"
                  "    }\n"
                  "\n"
                  "    // execution configuration\n"
                  "    const int GRID = (num_volumes + BLOCK_SIZE - 1) / BLOCK_SIZE;\n"
                  "\n"
                  "    size_t size = num_volumes * sizeof(real);\n"
                  "\n"
                  "    if(use_adpt_h)\n"
                  "        check_cuda_error(cudaMallocPitch((void **)&(solver->sv), &pitch_h, size, (size_t)NEQ + 3));\n"
                  "    else\n"
                  "        check_cuda_error(cudaMallocPitch((void **)&(solver->sv), &pitch_h, size, (size_t)NEQ));\n"
                  "\n"
                  "    check_cuda_error(cudaMemcpyToSymbol(pitch, &pitch_h, sizeof(size_t)));\n"
                  "\n"
                  "    kernel_set_model_initial_conditions<<<GRID, BLOCK_SIZE>>>(solver->sv, num_volumes);\n"
                  "\n"
                  "    check_cuda_error(cudaPeekAtLastError());\n"
                  "    cudaDeviceSynchronize();\n"
                  "    return pitch_h;\n"
                  "}\n\n", model_name);

    fprintf(file, "extern \"C\" SOLVE_MODEL_ODES(solve_model_odes_gpu) {\n"
                  "\n"
                  "    size_t num_cells_to_solve = ode_solver->num_cells_to_solve;\n"
                  "    uint32_t * cells_to_solve = ode_solver->cells_to_solve;\n"
                  "    real *sv = ode_solver->sv;\n"
                  "    real dt = ode_solver->min_dt;\n"
                  "    uint32_t num_steps = ode_solver->num_steps;\n"
                  "\n"
                  "    // execution configuration\n"
                  "    const int GRID = ((int)num_cells_to_solve + BLOCK_SIZE - 1) / BLOCK_SIZE;\n"
                  "\n"
                  "    size_t stim_currents_size = sizeof(real) * num_cells_to_solve;\n"
                  "    size_t cells_to_solve_size = sizeof(uint32_t) * num_cells_to_solve;\n"
                  "\n"
                  "    real *stims_currents_device;\n"
                  "    check_cuda_error(cudaMalloc((void **)&stims_currents_device, stim_currents_size));\n"
                  "    check_cuda_error(cudaMemcpy(stims_currents_device, stim_currents, stim_currents_size, cudaMemcpyHostToDevice));\n"
                  "\n"
                  "    // the array cells to solve is passed when we are using and adapative mesh\n"
                  "    uint32_t *cells_to_solve_device = NULL;\n"
                  "    if(cells_to_solve != NULL) {\n"
                  "        check_cuda_error(cudaMalloc((void **)&cells_to_solve_device, cells_to_solve_size));\n"
                  "        check_cuda_error(\n"
                  "            cudaMemcpy(cells_to_solve_device, cells_to_solve, cells_to_solve_size, cudaMemcpyHostToDevice));\n"
                  "    }\n"
                  "\n"
                  "    solve_gpu<<<GRID, BLOCK_SIZE>>>(current_t, dt, sv, stims_currents_device, cells_to_solve_device, num_cells_to_solve,\n"
                  "                                    num_steps);\n"
                  "\n"
                  "    check_cuda_error(cudaPeekAtLastError());\n"
                  "\n"
                  "    check_cuda_error(cudaFree(stims_currents_device));\n"
                  "    if(cells_to_solve_device)\n"
                  "        check_cuda_error(cudaFree(cells_to_solve_device));\n"
                  "}\n\n");

    fprintf(file, "__global__ void kernel_set_model_initial_conditions(real *sv, int num_volumes) {\n"
            "    int threadID = blockDim.x * blockIdx.x + threadIdx.x;\n"
            "\n"
            "    if (threadID < num_volumes) {\n\n");

    TokenNode *cur = rewind_token_list(difvarlist);
    int counter = 0;
    while (cur != NULL) {

        fprintf(file,
                "         *((real * )((char *) sv + pitch * %d) + threadID) = "
                        "%ef; //%s %s \n",
                counter, cur->initialvalue, cur->token.content, cur->units);
        cur = cur->next;
        counter++;
    }

    fprintf(file, "if(use_adpt) {\n"
                  "            *((real *)((char *)sv + pitch * %d) + threadID) = min_dt; // dt\n"
                  "            *((real *)((char *)sv + pitch * %d) + threadID) = 0.0;    // time_new\n"
                  "            *((real *)((char *)sv + pitch * %d) + threadID) = 0.0;    // previous dt\n"
                  "        }", counter-1, counter, counter+1);

    fprintf(file, "    }\n"
            "}\n\n");

    // SOLVE_MODEL_ODES_GPU
    fprintf(file, "// Solving the model for each cell in the tissue matrix ni x nj\n"
                  "__global__ void solve_gpu(real cur_time, real dt, real *sv, real *stim_currents, uint32_t *cells_to_solve,\n"
                  "                          uint32_t num_cells_to_solve, int num_steps) {\n"
                  "    int threadID = blockDim.x * blockIdx.x + threadIdx.x;\n"
                  "    int sv_id;\n"
                  "\n"
                  "    // Each thread solves one cell model\n"
                  "    if(threadID < num_cells_to_solve) {\n"
                  "        if(cells_to_solve)\n"
                  "            sv_id = cells_to_solve[threadID];\n"
                  "        else\n"
                  "            sv_id = threadID;\n"
                  "\n"
                  "        if(!use_adpt) {\n"
                  "            real rDY[NEQ];\n"
                  "\n"
                  "            for(int n = 0; n < num_steps; ++n) {\n"
                  "\n"
                  "                RHS_gpu(sv, rDY, stim_currents[threadID], sv_id, dt);\n"
                  "\n"
                  "                for(int i = 0; i < NEQ; i++) {\n"
                  "                    *((real *)((char *)sv + pitch * i) + sv_id) =\n"
                  "                        dt * rDY[i] + *((real *)((char *)sv + pitch * i) + sv_id);\n"
                  "                }\n"
                  "            }\n"
                  "        } else {\n"
                  "            solve_forward_euler_gpu_adpt(sv, stim_currents[threadID], cur_time + max_dt, sv_id);\n"
                  "        }\n"
                  "    }\n"
                  "}\n\n");


    fprintf(file, "inline __device__ void solve_forward_euler_gpu_adpt(real *sv, real stim_curr, real final_time, int thread_id) {\n"
                  "\n"
                  "    #define DT *((real *)((char *)sv + pitch * %d) + thread_id)\n"
                  "    #define TIME_NEW *((real *)((char *)sv + pitch * %d) + thread_id)\n"
                  "    #define PREVIOUS_DT *((real *)((char *)sv + pitch * %d) + thread_id)\n"
                  "\n"
                  "    real rDY[NEQ];\n"
                  "\n"
                  "    real _tolerances_[NEQ];\n"
                  "    real _aux_tol = 0.0;\n"
                  "    real dt = DT;\n"
                  "    real time_new = TIME_NEW;\n"
                  "    real previous_dt = PREVIOUS_DT;\n"
                  "\n"
                  "    real edos_old_aux_[NEQ];\n"
                  "    real edos_new_euler_[NEQ];\n"
                  "    real _k1__[NEQ];\n"
                  "    real _k2__[NEQ];\n"
                  "    real _k_aux__[NEQ];\n"
                  "    real sv_local[NEQ];\n"
                  "\n"
                  "    const real _beta_safety_ = 0.8;\n"
                  "\n"
                  "    const real __tiny_ = pow(abstol, 2.0f);\n"
                  "\n"
                  "    // dt = ((time_new + dt) > final_time) ? (final_time - time_new) : dt;\n"
                  "    if(time_new + dt > final_time) {\n"
                  "        dt = final_time - time_new;\n"
                  "    }\n"
                  "\n"
                  "    //#pragma unroll\n"
                  "    for(int i = 0; i < NEQ; i++) {\n"
                  "        sv_local[i] = *((real *)((char *)sv + pitch * i) + thread_id);\n"
                  "    }\n"
                  "\n"
                  "    RHS_gpu(sv_local, rDY, stim_curr, thread_id, dt);\n"
                  "    time_new += dt;\n"
                  "\n"
                  "    //#pragma unroll\n"
                  "    for(int i = 0; i < NEQ; i++) {\n"
                  "        _k1__[i] = rDY[i];\n"
                  "    }\n"
                  "\n"
                  "    int count = 0;\n"
                  "\n"
                  "    int count_limit = (final_time - time_new) / min_dt;\n"
                  "\n"
                  "    int aux_count_limit = count_limit + 2000000;\n"
                  "\n"
                  "    if(aux_count_limit > 0) {\n"
                  "        count_limit = aux_count_limit;\n"
                  "    }\n"
                  "\n"
                  "    while(1) {\n"
                  "\n"
                  "        for(int i = 0; i < NEQ; i++) {\n"
                  "            // stores the old variables in a vector\n"
                  "            edos_old_aux_[i] = sv_local[i];\n"
                  "            // //computes euler method\n"
                  "            edos_new_euler_[i] = _k1__[i] * dt + edos_old_aux_[i];\n"
                  "            // steps ahead to compute the rk2 method\n"
                  "            sv_local[i] = edos_new_euler_[i];\n"
                  "        }\n"
                  "\n"
                  "        time_new += dt;\n"
                  "\n"
                  "        RHS_gpu(sv_local, rDY, stim_curr, thread_id, dt);\n"
                  "        time_new -= dt; // step back\n"
                  "\n"
                  "        real greatestError = 0.0, auxError = 0.0;\n"
                  "        for(int i = 0; i < NEQ; i++) {\n"
                  "\n"
                  "            // stores the new evaluation\n"
                  "            _k2__[i] = rDY[i];\n"
                  "            _aux_tol = fabs(edos_new_euler_[i]) * reltol;\n"
                  "            _tolerances_[i] = (abstol > _aux_tol) ? abstol : _aux_tol;\n"
                  "\n"
                  "            // finds the greatest error between  the steps\n"
                  "            auxError = fabs(((dt / 2.0) * (_k1__[i] - _k2__[i])) / _tolerances_[i]);\n"
                  "\n"
                  "            greatestError = (auxError > greatestError) ? auxError : greatestError;\n"
                  "        }\n"
                  "\n"
                  "        /// adapt the time step\n"
                  "        greatestError += __tiny_;\n"
                  "        previous_dt = dt;\n"
                  "        /// adapt the time step\n"
                  "        dt = _beta_safety_ * dt * sqrt(1.0f / greatestError);\n"
                  "\n"
                  "        if(time_new + dt > final_time) {\n"
                  "            dt = final_time - time_new;\n"
                  "        }\n"
                  "\n"
                  "        // it doesn't accept the solution\n"
                  "        if(count < count_limit && (greatestError >= 1.0f)) {\n"
                  "            // restore the old values to do it again\n"
                  "            for(int i = 0; i < NEQ; i++) {\n"
                  "                sv_local[i] = edos_old_aux_[i];\n"
                  "            }\n"
                  "            count++;\n"
                  "            // throw the results away and compute again\n"
                  "        } else {\n"
                  "            count = 0;\n"
                  "\n"
                  "            if(dt < min_dt) {\n"
                  "                dt = min_dt;\n"
                  "            }\n"
                  "\n"
                  "            else if(dt > max_dt && max_dt != 0) {\n"
                  "                dt = max_dt;\n"
                  "            }\n"
                  "\n"
                  "            if(time_new + dt > final_time) {\n"
                  "                dt = final_time - time_new;\n"
                  "            }\n"
                  "\n"
                  "            // change vectors k1 e k2 , para que k2 seja aproveitado como k1 na proxima iteração\n"
                  "            for(int i = 0; i < NEQ; i++) {\n"
                  "                _k_aux__[i] = _k2__[i];\n"
                  "                _k2__[i] = _k1__[i];\n"
                  "                _k1__[i] = _k_aux__[i];\n"
                  "            }\n"
                  "\n"
                  "            // it steps the method ahead, with euler solution\n"
                  "            for(int i = 0; i < NEQ; i++) {\n"
                  "                sv_local[i] = edos_new_euler_[i];\n"
                  "            }\n"
                  "\n"
                  "            if(time_new + previous_dt >= final_time) {\n"
                  "                if((fabs(final_time - time_new) < 1.0e-5)) {\n"
                  "                    break;\n"
                  "                } else if(time_new < final_time) {\n"
                  "                    dt = previous_dt = final_time - time_new;\n"
                  "                    time_new += previous_dt;\n"
                  "                    break;\n"
                  "                } else {\n"
                  "                    dt = previous_dt = min_dt;\n"
                  "                    time_new += (final_time - time_new);\n"
                  "                    printf(\"Error: %%d: %%lf\\n\", thread_id, final_time - time_new);\n"
                  "                    break;\n"
                  "                }\n"
                  "            } else {\n"
                  "                time_new += previous_dt;\n"
                  "            }\n"
                  "        }\n"
                  "    }\n"
                  "\n"
                  "    //#pragma unroll\n"
                  "    for(int i = 0; i < NEQ; i++) {\n"
                  "        *((real *)((char *)sv + pitch * i) + thread_id) = sv_local[i];\n"
                  "    }\n"
                  "\n"
                  "    DT = dt;\n"
                  "    TIME_NEW = time_new;\n"
                  "    PREVIOUS_DT = previous_dt;\n"
                  "}\n\n", counter-1, counter, counter+1);


    // RHS CPU
    fprintf(file, "\n\ninline __device__ void RHS_gpu(real *sv, real *rDY, real stim_current, int thread_id, real dt) {\n\n");

    fprintf(file, "    //State variables\n");

    cur = rewind_token_list(difvarlist);
    counter = 0;
    while (cur != NULL) {
        fprintf(file, "    real %s; //%s ", cur->token.content, cur->units);
        cur = cur->next;
        counter++;
    }

    fprintf(file, "\n\n    if(use_adpt) {\n");
    cur = rewind_token_list(difvarlist);
    counter = 0;
    while (cur != NULL) {
        fprintf(file,
                "        real %s_old_ =  sv[%d];\n", cur->token.content, counter);
        cur = cur->next;
        counter++;
    }
    fprintf(file, "    } else {\n");
    cur = rewind_token_list(difvarlist);
    counter = 0;
    while (cur != NULL) {
        fprintf(file,
                "        %s_old_ =  *((real*)((char*)sv_ + pitch * %d) + "
                "thread_id);\n",
                cur->token.content, counter);
        cur = cur->next;
        counter++;
    }
    fprintf(file, "    }\n\n");


    fprintf(file, "    #include \"%s_common.inc.c\"", model_name);


    fprintf(file, "\n\n}\n\n");

    fclose(file);
    sdsfree(headername);
    sdsfree(filename);
}

void generate_single_cell_solver(sds model_name) {

    if (eq_counter <= 0) {
        printf("ERROR - Equation not found\n");
        exit(1);
    }

    preced_alg_list = create_preced_alg_list(rewind_alg_list(alglist));
    AlgList *cural;
    TokenNode *resolved_dep_list = NULL;
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
    filename = sdscat(filename, "_single_cell_solver.c");

    file = fopen(filename, "w");
    fprintf(file, "#include <cvode/cvode.h>\n"
                  "#include <math.h>\n"
                  "#include <nvector/nvector_serial.h>\n"
                  "#include <stdbool.h>\n"
                  "#include <stdio.h>\n"
                  "#include <stdlib.h>\n"
                  "#include <sundials/sundials_dense.h>\n"
                  "#include <sundials/sundials_types.h>\n"
                  "#include <sunlinsol/sunlinsol_dense.h> \n"
                  "#include <sunmatrix/sunmatrix_dense.h>"
                  " \n\n");

    // SET INITIAL CONDITIONS CPU
    fprintf(file,
            "void set_initial_conditions(N_Vector x0) { \n\n");
    TokenNode *cur = rewind_token_list(difvarlist);
    int counter = 0;
    while (cur != NULL) {
        fprintf(file, "    \tNV_Ith_S(x0, %d) = %ef; //%s %s \n", counter, cur->initialvalue,
                cur->token.content, cur->units);
        cur = cur->next;
        counter++;
    }
    fprintf(file, "}\n\n");

    fprintf(file, "#define NEQ %d\n"
                  "typedef realtype real;\n"
                  "\n"
                  "\n"
                  "realtype stim_start = 2.0;\n"
                  "realtype stim_dur = 1.0;\n"
                  "realtype stim_current;", counter);

    print_if(file, iflist);

    // RHS CPU
    fprintf(file, "\n\n static int %s(realtype t, N_Vector sv, N_Vector rDY, void *f_data) {\n\n", model_name);
    fprintf(file, "stim_current = 0.0;\n"
                  "\n"
                  "\tif(t <= stim_start + stim_dur) {\n"
                  "\t\tstim_current = -53;\n"
                  "\t}\n"
                  "");

    fprintf(file, "    //State variables\n");
    cur = rewind_token_list(difvarlist);
    counter = 0;
    while (cur != NULL) {
        fprintf(file,
                "    const real %s_old_ =  NV_Ith_S(sv, %d);\n", cur->token.content, counter);
        cur = cur->next;
        counter++;
    }

    fprintf(file, "\n");

    fprintf(file, "    //Parameters\n");
    cur = rewind_token_list(parvarlist);
    while (cur != NULL) {
        fprintf(file, "    const real %s = %.15ef;\n", cur->token.content,
                cur->initialvalue);
        cur = cur->next;
    }

    print_right_alg(file, alglist, resolved_dep_list);

    print_diff_cvode(file, difflist);

    fprintf(file, "\n\treturn 0;  \n\n}\n\n");

    fprintf(file, "static int check_flag(void *flagvalue, const char *funcname, int opt) {\n"
                  "\n"
                  "    int *errflag;\n"
                  "\n"
                  "    /* Check if SUNDIALS function returned NULL pointer - no memory allocated */\n"
                  "    if(opt == 0 && flagvalue == NULL) {\n"
                  "\n"
                  "        fprintf(stderr, \"\\nSUNDIALS_ERROR: %%s() failed - returned NULL pointer\\n\\n\", funcname);\n"
                  "        return (1);\n"
                  "    }\n"
                  "\n"
                  "    /* Check if flag < 0 */\n"
                  "    else if(opt == 1) {\n"
                  "        errflag = (int *)flagvalue;\n"
                  "        if(*errflag < 0) {\n"
                  "            fprintf(stderr, \"\\nSUNDIALS_ERROR: %%s() failed with flag = %%d\\n\\n\", funcname, *errflag);\n"
                  "            return (1);\n"
                  "        }\n"
                  "    }\n"
                  "\n"
                  "    /* Check if function returned NULL pointer - no memory allocated */\n"
                  "    else if(opt == 2 && flagvalue == NULL) {\n"
                  "        fprintf(stderr, \"\\nMEMORY_ERROR: %%s() failed - returned NULL pointer\\n\\n\", funcname);\n"
                  "        return (1);\n"
                  "    }\n"
                  "\n"
                  "    return (0);\n"
                  "}\n");

    fprintf(file, "void solve_ode(N_Vector y, float final_t) {\n"
                  "\n"
                  "    void *cvode_mem = NULL;\n"
                  "    int flag;\n"
                  "\n"
                  "    // Set up solver\n"
                  "    cvode_mem = CVodeCreate(CV_BDF);\n"
                  "\n"
                  "    if(cvode_mem == 0) {\n"
                  "        fprintf(stderr, \"Error in CVodeMalloc: could not allocate\\n\");\n"
                  "        return;\n"
                  "    }\n"
                  "\n"
                  "    flag = CVodeInit(cvode_mem, %s, 0, y);\n"
                  "    if(check_flag(&flag, \"CVodeInit\", 1))\n"
                  "        return;\n"
                  "\n"
                  "    flag = CVodeSStolerances(cvode_mem, 1.49012e-8, 1.49012e-8);\n"
                  "    if(check_flag(&flag, \"CVodeSStolerances\", 1))\n"
                  "        return;\n"
                  "\n"
                  "    /* Provide RHS flag as user data which can be access in user provided routines */\n"
                  "   // flag = CVodeSetUserData(cvode_mem, (void *)&params);\n"
                  "    //if(check_flag(&flag, \"CVodeSetUserData\", 1))\n"
                  "    //    return;\n"
                  "\n"
                  "    // Create dense SUNMatrix for use in linear solver\n"
                  "    SUNMatrix A = SUNDenseMatrix(NEQ, NEQ);\n"
                  "    if(check_flag((void *)A, \"SUNDenseMatrix\", 0))\n"
                  "        return;\n"
                  "\n"
                  "    // Create dense linear solver for use by CVode\n"
                  "    SUNLinearSolver LS = SUNLinSol_Dense(y, A);\n"
                  "    if(check_flag((void *)LS, \"SUNLinSol_Dense\", 0))\n"
                  "        return;\n"
                  "\n"
                  "    // Attach the linear solver and matrix to CVode by calling CVodeSetLinearSolver\n"
                  "    flag = CVodeSetLinearSolver(cvode_mem, LS, A);\n"
                  "    if(check_flag((void *)&flag, \"CVodeSetLinearSolver\", 1))\n"
                  "        return;\n"
                  "\n"
                  "\trealtype dt=0.01;\n"
                  "    realtype tout = dt;\n"
                  "    int retval;\n"
                  "    realtype t;\n"
                  "\n"
                  "\tFILE *f = fopen(\"out.txt\", \"w\");\n"
                  "\n"
                  "\twhile(tout < final_t) {\n"
                  "\n"
                  "\t\tretval = CVode(cvode_mem, tout, y, &t, CV_NORMAL);\n"
                  "\n"
                  "\t\tif(retval == CV_SUCCESS) {\t\t\t\n"
                  "\t        fprintf(f, \"%%lf \", t);\n"
                  "\t\t\tfor(int i = 0; i < NEQ; i++) {\n"
                  "\t        \tfprintf(f, \"%%lf \", NV_Ith_S(y,i));\n"
                  "\t\t\t} \n"
                  "\t        \n"
                  "\t\t\tfprintf(f, \"\\n\");\n"
                  "\n"
                  "\t\t\ttout+=dt;\n"
                  "\t\t}\n"
                  "\n"
                  "\t}\n"
                  "\n"
                  "    // Free the linear solver memory\n"
                  "    SUNLinSolFree(LS);\n"
                  "    SUNMatDestroy(A);\n"
                  "    CVodeFree(&cvode_mem);\n"
                  "}\n", model_name);


    fprintf(file, "\nint main(int argc, char **argv) {\n"
                  "\n"
                  "\tN_Vector x0 = N_VNew_Serial(NEQ);\n"
                  "\n"
                  "\tset_initial_conditions(x0);\n"
                  "\n"
                  "\tsolve_ode(x0, strtod(argv[1], NULL));\n"
                  "\n"
                  "\n"
                  "\treturn (0);\n"
                  "}");


    fclose(file);
    sdsfree(filename);
}

// 08/03/2008 ///////
int print_right_alg(FILE *file, AlgList *list, TokenNode *orderedlist) {
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

        fprintf(file, "\n    real calc_%s = ", curalg->eq->token.content);
        cur = curalg->eq;
        cur = cur->next->next;
        if (strcmp(curalg->eq->token.content, (const char *)"i_Stim") == 0) {
            fprintf(file, "stim_current");
        } else {
            //HACK: we should do this better :)
            sds tmp = sdsnew("");
            tmp = sdscatfmt(tmp, "calc_%s", curalg->eq->token.content);

            print_eq(file, cur, tmp);
        }

        fprintf(file, ";\t//%d", curalg->number);
        curl = curl->next;
    }
    fprintf(file, "\n");
    return 0;
}

#endif //_COMPILE_H_

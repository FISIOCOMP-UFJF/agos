#ifndef _FSM_H_
#define _FSM_H_

#include "fsm_util.h"
#include "lex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define SUCCESS 0
#define INVALID 1

#include "string/sds.h"

struct if_else {
    sds if_condition;
    sds if_statements;

    sds *else_if_conditions;
    sds *else_if_statements;

    sds else_statements;
};

int math = 0; // indication of <math>'s founded

Token stack[100];
int s_ind = -1;
int ifs = 0;

int princ();

int C();  // C -> <math> P ... end
int P();  // P -> <apply> = E E cpar | <apply> = D E cpar
int D();  // D -> <apply> diff vari bvar vari cpar cpar | <apply> diff bvar vari
// cpar vari cpar
int E();  // E -> <apply> p_op E E... cpar | <apply> i_op E E... cpar | <apply>
// piop E... cpar | opar Q | numb | vari
int F();  // F -> <piecewise> I <otherwise> E cpar cpar
int R();  // R -> <apply> R_OP E E cpar
int L();  // L -> <apply> L_OP L L... cpar | R
int Q();  // Q -> root E degree E cpar cpar | root degree E cpar E cpar | root E
// cpar
int M();  // procura components ou math
int DV(); // D -> <apply> diff vari bvar vari cpar cpar | <apply> diff bvar vari
// cpar vari cpar // to be used inside an equation// its the same
// syntax of the D but have different semantics
int I();  // IF I | IF
int IF(); //<piece> E L cpar

void push(Token op);

Token pop();

int empty();

xmlNode *get_component(xmlNode *node);

xmlNode *get_math(xmlNode *node);

void error(char *msg, xmlNode *errpointer);

int princ() {
    int state = 0;
    Token token;
    memset(&token, 0, sizeof(Token));
    while (state != 2) {
        if (state == 0) {
            if (M() == SUCCESS)
                state = 2;
            else {
                return INVALID;
            }
        }
    }
    return SUCCESS;
}

int C() {
    int state = 0;
    Token token;
    memset(&token, 0, sizeof(Token));
    while (state != 3) {
        switch (state) {
            case 0: {
                xmlNode *errpointer = pointer;
                token = lex();

                if (token.type == MATH)
                    state = 1;
                else {
                    error((char *) "math expected", errpointer);
                    return INVALID;
                }
                break;
            }
            case 1: {
                if (P() == SUCCESS) {
                    state = 2;
                }
                else {
                    return INVALID;
                }
                break;
            }
            case 2: {
                xmlNode *errpointer = pointer;
                if (P() == SUCCESS)
                    state = 2;
                else {

                    token = lex();
                    if (token.type == CPAR)
                        state = 3;
                    else {
                        error((char *) "</math> expected", errpointer);
                        return INVALID;
                    }
                }
                break;
            }
            default: fprintf(stderr, "Error on Line %d\n", __LINE__);
        }
    }
    return SUCCESS;
}

int P() {
    int state = 0;
    Token token;
    memset(&token, 0, sizeof(Token));
    while (state != 5) {
        switch (state) {
            case 0: {
                xmlNode *auxpointer = pointer;
                int auxindex = lex_index;
                token = lex();
                if (token.type == OPAR)
                    state = 1;
                else {
                    pointer = auxpointer;
                    lex_index = auxindex;
                    return INVALID;
                }
                break;
            }
            case 1: {
                xmlNode *errpointer = pointer;
                token = lex();
                if (!strcmp(token.content, "=")) {
                    state = 2;
                    push(token);
                } else {
                    error((char *) "<eq/> expected", errpointer);
                    return INVALID;
                }
                break;
            }
            case 2: {
                xmlNode *errpointer = pointer;
                if (D() != INVALID) {
                    pop();
                    state = 3;

                } else {

                    token = lex();
                    if (token.type == VARI) {
                        algvarlist = add_list(token, algvarlist);
                        eqpointer = concat_token(token, NULL);
                        alglist = attach_alg_eq(eqpointer, alglist);
                        eqpointer = concat_token(pop(), eqpointer);
                        state = 3;
                    } else {
                        error((char *) "<ci> or differential equation expected", errpointer);
                        return INVALID;
                    }
                }
                break;
            }
            case 3: {
                xmlNode *auxpointer = pointer;
                int indx = lex_index;

                if (E() == SUCCESS) {
                    state = 4;
                } else {
                    pointer = auxpointer;
                    lex_index = indx;
                    if (!empty())
                        pop();

                    // creating a *char with the if number specification
                    char *ifn = (char *) calloc(4, sizeof(char));
                    sprintf(ifn, "%d", ifs);
                    ifs++;
                    Token t;
                    t.content = ifn;
                    t.tag = (char *) "";
                    t.type = PI_W;
                    eqpointer = concat_token(t, eqpointer);
                    eqpointer = NULL;
                    if (F() == SUCCESS) {
                        state = 4;
                    } else {
                        return INVALID;
                    }
                }
                break;
            }
            case 4: {
                xmlNode *errpointer = pointer;
                token = lex();

                if (token.type == CPAR) {
                    // imprime ; ao fim da expressao
                    state = 5;
                } else {
                    error((char *) "</apply> expected", errpointer);
                    return INVALID;
                }
                break;
            }

            default: fprintf(stderr, "Error on Line %d\n", __LINE__);
        }
    }
    return SUCCESS;
}

int D() {
    DiffHeader *diff = NULL;
    int state = 0;
    Token token;
    memset(&token, 0, sizeof(Token));
    while (state != 10) {
        switch (state) {
            case 0: {
                xmlNode *auxpointer = pointer;
                int auxi = lex_index;
                token = lex();
                if (token.type == OPAR)
                    state = 1;
                else {
                    pointer = auxpointer;
                    lex_index = auxi;
                    return INVALID;
                }
            }
                break;
            case 1:
                token = lex();
                if (token.type == DIFF)
                    state = 2;
                else
                    return INVALID;
                break;
            case 2: {
                xmlNode *errpointer = pointer;
                diff = (DiffHeader *) calloc(1, sizeof(DiffHeader));
                token = lex();

                if (token.type == VARI) {
                    diff->diffvar = token;
                    state = 3;
                } else {
                    if (token.type == BVAR) {
                        state = 4;
                    } else {
                        error((char *) "<bvar> or <ci> expected", errpointer);
                        return INVALID;
                    }
                }
            }
                break;
            case 3: {
                xmlNode *errpointer = pointer;
                token = lex();
                if (token.type == BVAR)
                    state = 6;
                else {
                    error((char *) "<bvar> expected", errpointer);
                    return INVALID;
                }
            }
                break;
            case 4: {
                xmlNode *errpointer = pointer;
                token = lex();
                if (token.type == VARI) {
                    diff->freevar = token;
                    state = 5;
                } else {
                    error((char *) "<ci> expected", errpointer);
                    return INVALID;
                }
            }
                break;
            case 5: {
                xmlNode *errpointer = pointer;
                token = lex();
                if (token.type == CPAR)
                    state = 7;
                else {
                    error((char *) "</apply> expected", errpointer);
                    return INVALID;
                }
            }
                break;
            case 6: {
                xmlNode *errpointer = pointer;
                token = lex();
                if (token.type == VARI) {
                    diff->freevar = token;
                    diff->eq = concat_token(stack[s_ind], diff->eq);
                    eqpointer = diff->eq;
                    state = 8;
                } else {
                    error((char *) "<ci> expected", errpointer);
                    return INVALID;
                }
            }
                break;
            case 7: {
                xmlNode *errpointer = pointer;
                token = lex();
                // printf("token.type == %d  %s\n",token.type, token.tag);
                if (token.type == VARI) {
                    diff->diffvar = token;
                    // concatenating the equal operator
                    diff->eq = concat_token(stack[s_ind], diff->eq);
                    eqpointer = diff->eq;
                    state = 9;
                } else {
                    error((char *) "<ci> expected", errpointer);
                    return INVALID;
                }
            }
                break;
            case 8:
                token = lex();
                if (token.type == CPAR)
                    state = 9;
                else
                    return INVALID;
                break;
            case 9:
                token = lex();
                if (token.type == CPAR) {
                    difvarlist = add_list(diff->diffvar, difvarlist);
                    state = 10;
                } else
                    return INVALID;
                break;
            default:
                printf("ERROR - fsm P\n");
                break;
        }
    }
    difflist = attach_diff_eq(diff, difflist);

    return SUCCESS;
}

int E() {
    int estado = 0;
    Token token;
    memset(&token, 0, sizeof(Token));
    while (estado != 5) {
        switch (estado) {
            case 0: {
                xmlNode *auxpointer = pointer;
                int auxindex = lex_index;

                if (DV() == SUCCESS) {

                    estado = 5;
                } else {
                    pointer = auxpointer;
                    lex_index = auxindex;
                    token = lex();
                    if (token.type == OPAR) {
                        xmlNode *auxp = pointer;
                        int indl = lex_index;
                        Token t = lex();
                        if (t.type != P_OP && t.type != ROOT) {
                            eqpointer = concat_token(token, eqpointer);
                        }
                        pointer = auxp;
                        lex_index = indl;
                        estado = 1;
                    } else if (token.type == NUMB || token.type == VARI) {

                        if (!empty()) {
                            if (pointer == NULL) {
                                eqpointer = concat_token(token, eqpointer);
                            } else {
                                if (next(pointer) != NULL) {
                                    eqpointer = concat_token(token, eqpointer);
                                    eqpointer = concat_token(stack[s_ind], eqpointer);
                                } else {
                                    eqpointer = concat_token(token, eqpointer);
                                }
                            }
                        } else {
                            eqpointer = concat_token(token, eqpointer);
                        }
                        estado = 5;
                    } else if (token.type == CPAR) {

                        pointer = auxpointer;
                        lex_index = auxindex;
                        return INVALID;
                    } else
                        return INVALID;
                }
            }
                break;
            case 1: {

                xmlNode *auxpointer = pointer;
                int indx = lex_index;
                token = lex();
                if (token.type == I_OP) {
                    // caso seja um mod % truncar o valor com (int)
                    if (!strcmp(token.content, "%")) {
                        Token taux;
                        taux.content = (char *) "(int)";
                        taux.type = P_OP;
                        eqpointer = concat_token(taux, eqpointer);
                    }

                    push(token);
                    estado = 2;
                } else if (token.type == PIOP) {
                    push(token);
                    estado = 6;
                } else // operadores prefixos
                if (token.type == P_OP) {
                    eqpointer = concat_token(token, eqpointer);
                    Token auxt;
                    memset(&token, 0, sizeof(Token));
                    auxt.content = (char *) ",";
                    push(auxt);
                    estado = 7;
                } else {
                    pointer = auxpointer;
                    lex_index = indx;
                    if (Q() == SUCCESS)
                        estado = 5;
                    else {
                        error((char *) "prefix, infix, <minus/> or <root/> expected",
                              auxpointer);
                        return INVALID;
                    }
                }
            }
                break;
            case 2:
                if (E() == SUCCESS)
                    estado = 3;
                else
                    return INVALID;
            case 3:
                if (stack[s_ind].content != NULL)
                    if (!strcmp(stack[s_ind].content, "%")) {
                        Token taux;
                        taux.content = (char *) "(int)";
                        taux.type = P_OP;
                        eqpointer = concat_token(taux, eqpointer);
                    }
                if (E() == SUCCESS)
                    estado = 4;
                else
                    return INVALID;
                break;
            case 4: {
                xmlNode *errpointer = pointer;

                xmlNode *auxpointer = pointer;
                int auxindex = lex_index;
                Token tx = lex();

                if (stack[s_ind].content != NULL) {
                    if (tx.type != CPAR && !strcmp(stack[s_ind].content, "%")) {
                        tx.content = (char *) "(int)";
                        tx.type = P_OP;
                        eqpointer = concat_token(tx, eqpointer);
                    }
                }

                pointer = auxpointer;
                lex_index = auxindex;
                if (E() == SUCCESS) {
                    estado = 4;
                } else {
                    token = lex();
                    if (token.type == CPAR) {
                        estado = 5;
                        pop();
                        if (!empty() && next(pointer) != NULL) {
                            eqpointer = concat_token(token, eqpointer);
                            eqpointer = concat_token(stack[s_ind], eqpointer);
                        } else {
                            eqpointer = concat_token(token, eqpointer);
                        }
                    } else {
                        error((char *) "</apply> expected", errpointer);
                        return INVALID;
                    }
                }
            }
                break;
            case 6: {
                xmlNode *auxpointer = next(pointer);

                if (auxpointer != NULL) {
                    auxpointer = auxpointer->next;
                    if (next(auxpointer) == NULL) {
                        eqpointer = concat_token(stack[s_ind], eqpointer);
                    }
                }
                if (E() == SUCCESS) {
                    estado = 4;
                } else
                    return INVALID;
            }
                break;
            case 7:
                if (E() == SUCCESS)
                    estado = 4;
                break;

            default:
                printf("ERROR - fsm E\n");
                break;
        }
    }
    return SUCCESS;
}

int F() {
    int estado = 0;
    IfHeader *ifheader = NULL;
    Token token;
    memset(&token, 0, sizeof(Token));
    while (estado != 9) {
        switch (estado) {
            case 0:

                token = lex();
                if (token.type == PI_W) {
                    p_if_counter++;
                    estado = 1;
                } else
                    return INVALID;
                break;
            case 1: {
                xmlNode *errpointer = pointer;
                xmlNode *auxpointer = pointer;
                int auxindex = lex_index;
                token = lex();

                if (token.type != PIEC) {
                    error((char *) "<piece> expected", errpointer);
                    return INVALID;
                }
                pointer = auxpointer;
                lex_index = auxindex;

                if (I() == SUCCESS) {
                    estado = 5;
                }
            }
                break;
            case 5: {
                xmlNode *errpointer = pointer;
                token = lex();
                if (token.type == OT_W) {
                    estado = 6;
                } else {
                    error((char *) "<otherwise> expected", errpointer);
                    return INVALID;
                }
            }
                break;
            case 6: {
                xmlNode *auxpointer = pointer;
                int auxindex = lex_index;
                if (E() == SUCCESS) {
                    TokenNode *taux = eqpointer;
                    while (taux->prev != NULL)
                        taux = taux->prev;
                    IfList *cur = iflist;

                    while (cur->next != NULL)
                        cur = cur->next;

                    ifheader = cur->ifheader;
                    ifheader->other = taux;
                    eqpointer = NULL;
                    estado = 7;
                } else {
                    pointer = auxpointer;
                    lex_index = auxindex;
                    pop();

                    // creating a *char with the if number specification
                    char *ifn = (char *) calloc(4, sizeof(char));
                    sprintf(ifn, "%d", ifs);
                    ifs++;
                    Token t;
                    t.content = ifn;
                    t.tag = (char *) "";
                    t.type = PI_W;
                    eqpointer = concat_token(t, eqpointer);
                    ifheader->other = eqpointer;
                    eqpointer = NULL;
                    if (F() == SUCCESS) {
                        estado = 7;
                    } else {
                        error((char *) "expression or coditional statement expected",
                              auxpointer);
                        return INVALID;
                    }
                }
            }
                break;
            case 7: {
                xmlNode *errpointer = pointer;
                token = lex();
                if (token.type == CPAR) {
                    estado = 8;
                } else {
                    error((char *) "</otherwise> expected", errpointer);
                    return INVALID;
                }
            }
                break;
            case 8: {
                xmlNode *errpointer = pointer;
                token = lex();
                if (token.type == CPAR) {
                    estado = 9;
                } else {
                    error((char *) "</piecewise> expected", errpointer);
                    return INVALID;
                }
            }
                break;
            default: fprintf(stderr, "Error on Line %d\n", __LINE__);
        }
    }
    return SUCCESS;
}

int I() {

    int state = 0;
    Token token;
    memset(&token, 0, sizeof(Token));
    while (state != 2) {
        switch (state) {
            case 0: {
                if (IF() == SUCCESS) {
                    state = 1;
                } else {

                    state = 2;
                }
                break;
            }
            case 1: {
                if (I() == SUCCESS) {
                    state = 2;
                }
                break;
            }
            default: fprintf(stderr, "Error on Line %d\n", __LINE__);
        }
    }
    return SUCCESS;
}

int IF() {

    int state = 0;
    IfHeader *ifheader = NULL;
    Token token;
    memset(&token, 0, sizeof(Token));
    while (state != 4) {
        switch (state) {
            case 0: {
                xmlNode *auxpointer = pointer;
                int auxindex = lex_index;
                token = lex();
                if (token.type == PIEC) {
                    ifheader = (IfHeader *) calloc(1, sizeof(IfHeader));
                    ifheader->if_counter = p_if_counter;
                    iflist = attach_if_eq(ifheader, iflist);
                    state = 1;
                } else {

                    pointer = auxpointer;
                    lex_index = auxindex;
                    return INVALID;
                }
                break;
            }
            case 1: {
                xmlNode *auxpointer = pointer;
                int auxindex = lex_index;
                if (E() == SUCCESS) {
                    TokenNode *taux = eqpointer;
                    while (taux->prev != NULL)
                        taux = taux->prev;
                    ifheader->piece = taux;
                    eqpointer = NULL;
                    state = 2;
                } else {
                    pointer = auxpointer;
                    lex_index = auxindex;
                    pop();

                    char *ifn = (char *) calloc(4, sizeof(char));
                    sprintf(ifn, "%d", ifs);
                    ifs++;
                    Token t;
                    t.content = ifn;
                    t.tag = (char *) "";
                    t.type = PI_W;
                    eqpointer = concat_token(t, eqpointer);
                    ifheader->piece = eqpointer;
                    eqpointer = NULL;
                    if (F() == SUCCESS) {
                        state = 3;
                    } else {
                        error((char *) "expression or conditional statement expected",
                              auxpointer);
                        return INVALID;
                    }
                }
                break;
            }
            case 2: {
                if (L() == SUCCESS) {
                    TokenNode *taux = eqpointer;
                    while (taux->prev != NULL)
                        taux = taux->prev;
                    ifheader->cond = taux;
                    eqpointer = NULL;
                    state = 3;
                } else
                    return INVALID;
                break;
            }
            case 3: {
                xmlNode *errpointer = pointer;
                token = lex();
                if (token.type == CPAR) {
                    state = 4;
                } else {
                    error((char *) "</piece> expected", errpointer);
                    return INVALID;
                }
                break;
            }
            default: fprintf(stderr, "Error on Line %d\n", __LINE__);
        }
    }
    return SUCCESS;
}

int R() {
    int estado = 0;
    Token token;
    memset(&token, 0, sizeof(Token));
    while (estado != 5) {
        switch (estado) {
            case 0:
                token = lex();

                if (token.type == OPAR) {
                    eqpointer = concat_token(token, eqpointer);
                    estado = 1;
                } else
                    return INVALID;
                break;
            case 1:
                token = lex();

                if (token.type == R_OP) {
                    if (!strcmp(token.content, "="))
                        token.content = (char *) "==";
                    push(token);
                    estado = 2;
                } else
                    return INVALID;
                break;
            case 2: {
                xmlNode *errpointer = pointer;
                if (E() == SUCCESS) {
                    // printf("TOKEN %s\n", token.content);
                    pop();
                    estado = 3;
                } else {
                    error((char *) "expression expected", errpointer);
                    return INVALID;
                }
            }
                break;
            case 3: {
                xmlNode *errpointer = pointer;
                if (E() == SUCCESS) {
                    estado = 4;
                } else {
                    error((char *) "expression expected", errpointer);
                    return INVALID;
                }
            }
                break;
            case 4: {
                xmlNode *errpointer = pointer;
                token = lex();
                if (token.type == CPAR) {
                    eqpointer = concat_token(token, eqpointer);
                    estado = 5;
                } else {
                    error((char *) "</apply> expected", errpointer);
                    return INVALID;
                }
            }
                break;
            default:
                printf("ERROR - fsm princ\n");
                break;
        }
    }
    return SUCCESS;
}

int L() {
    int isXor = 0;
    int estado = 0;
    Token token;
    memset(&token, 0, sizeof(Token));
    while (estado != 5) {
        switch (estado) {
            case 0: {
                xmlNode *auxpointer = pointer;
                int indx = lex_index;

                if (R() == SUCCESS) {

                    estado = 5;
                } else {

                    lex_index = indx;
                    pointer = auxpointer;
                    token = lex();
                    if (token.type == OPAR) {
                        // eqpointer = concat_token(token, eqpointer);
                        estado = 1;
                    } else {
                        lex_index = indx;
                        pointer = auxpointer;
                        //	error("logical or relational expression expected", auxpointer);
                        //************ 01/08/2006 **************
                        return INVALID;
                    }
                }
            }
                break;
            case 1: {
                xmlNode *errpointer = pointer;
                token = lex();
                if (token.type == L_OP) {
                    if (!strcmp(token.tag, "xor")) {
                        isXor = 1;
                        eqpointer = concat_token(token, eqpointer);
                        Token ntoken;
                        memset(&ntoken, 0, sizeof(Token));
                        ntoken.content = (char *) ",";
                        // printf(stack[s_ind].content);
                        push(ntoken);

                        // printf(stack[s_ind].content);

                    } else {

                        push(token);
                    }
                    estado = 2;
                } else {
                    error((char *) "<and/> or <or/> expected", errpointer);
                    return INVALID;
                }
            }
                break;
            case 2:

                if (L() == SUCCESS) {

                    /// FAZER AKI A INSERCAO DO OPERADOR LOGICO
                    eqpointer = concat_token(stack[s_ind], eqpointer);
                    estado = 3;
                } else
                    return INVALID;
                break;
            case 3:
                if (L() == SUCCESS) {
                    estado = 4;
                } else
                    return INVALID;
                break;
            case 4: {
                xmlNode *auxpointer = pointer;
                int indx = lex_index;
                TokenNode *auxep = eqpointer;
                if (L() == SUCCESS) {

                    attach_token(stack[s_ind], auxep);
                    estado = 4;
                } else {

                    pointer = auxpointer;
                    lex_index = indx;
                    token = lex();

                    pop();
                    if (token.type == CPAR) {
                        if (isXor) {
                            Token ctoken;
                            ctoken.content = (char *) ")";
                            eqpointer = concat_token(ctoken, eqpointer);
                        }
                        eqpointer = concat_token(token, eqpointer);
                        estado = 5;
                    } else {
                        error((char *) "</apply> expected", auxpointer);
                        return INVALID;
                    }
                }
            }
                break;
            default:
                printf("ERROR - fsm princ\n");
                break;
        }
    }

    return SUCCESS;
}

int Q() {
    int estado = 0;
    Token token;
    TokenNode *dgpointer = NULL;
    memset(&token, 0, sizeof(Token));
    while (estado != 6) {
        switch (estado) {
            case 0:
                token = lex();
                if (token.type == ROOT) {
                    eqpointer = concat_token(token, eqpointer);
                    estado = 1;
                } else
                    return INVALID;
                break;
            case 1: {
                xmlNode *auxpointer = pointer;
                int indx = lex_index;
                Token t;
                memset(&t, 0, sizeof(Token));
                t.content = (char *) ",";
                push(t);
                if (E() == SUCCESS) {
                    estado = 2;
                } else {
                    pointer = auxpointer;
                    lex_index = indx;
                    token = lex();
                    if (token.type == DGRE) {
                        estado = 7;
                    } else
                        return INVALID;
                }
                break;
            }
            case 2: {
                xmlNode *auxpointer = pointer;
                int indx = lex_index;
                token = lex();
                if (token.type == DGRE) {
                    char *aux = (char *) calloc(5, sizeof(char));
                    sprintf(aux, "1.0/");
                    Token t;
                    t.type = NUMB;
                    t.content = aux;
                    eqpointer = concat_token(t, eqpointer);
                    estado = 3;
                } else {

                    pointer = auxpointer;
                    lex_index = indx;

                    token = lex();
                    if (token.type == CPAR) {
                        Token t;
                        memset(&t, 0, sizeof(Token));
                        t.content = (char *) "1.0/2.0";
                        t.type = NUMB;
                        t.tag = (char *) "cn";
                        Token tok;
                        memset(&tok, 0, sizeof(Token));
                        tok.content = (char *) ")";
                        tok.type = CPAR;

                        eqpointer = concat_token(pop(), eqpointer);
                        eqpointer = concat_token(t, eqpointer);
                        eqpointer = concat_token(tok, eqpointer);

                        if (next(pointer) != NULL) {
                            eqpointer = concat_token(stack[s_ind], eqpointer); // concat_token(t, eqpointer);
                        }

                        estado = 6;
                    } else
                        return INVALID;
                }
            }
                break;
            case 3:
                if (E() == SUCCESS) {
                    estado = 4;
                } else
                    return INVALID;
                break;
            case 4:
                token = lex();
                if (token.type == CPAR) {
                    estado = 5;
                } else
                    return INVALID;
                break;
            case 5:
                token = lex();
                if (token.type == CPAR) {
                    char *aux = (char *) calloc(5, sizeof(char));
                    sprintf(aux, "1.0/");

                    eqpointer = concat_token(token, eqpointer);
                    pop(); // poping ','
                    if (next(pointer) != NULL)
                        eqpointer = concat_token(stack[s_ind], eqpointer);
                    estado = 6;
                } else
                    return INVALID;
                break;
            case 7:
                dgpointer = eqpointer;
                eqpointer = NULL;
                if (E() == SUCCESS) {
                    estado = 8;
                } else
                    return INVALID;
                break;
            case 8:
                token = lex();
                if (token.type == CPAR) {
                    TokenNode *auxp = dgpointer;

                    dgpointer = rewind_token_list(eqpointer);
                    eqpointer = auxp;
                    estado = 9;
                } else
                    return INVALID;
                break;
            case 9:
                if (E() == SUCCESS) {
                    Token t;
                    t.content = (char *) ",";
                    eqpointer = concat_token(t, eqpointer);
                    eqpointer->next = dgpointer;
                    dgpointer->prev = eqpointer;
                    while (eqpointer->next != NULL)
                        eqpointer = eqpointer->next;
                    estado = 10;
                } else
                    return INVALID;
                break;
            case 10:
                token = lex();
                if (token.type == CPAR) {
                    // NESTE PONTO Eh POSSIVEL CRIAR O POW3 ...
                    eqpointer = concat_token(token, eqpointer);
                    pop(); // poping ','
                    if (next(pointer) != NULL)
                        eqpointer = concat_token(stack[s_ind], eqpointer);
                    estado = 6;
                } else
                    return INVALID;
                break;
            default:
                printf("ERROR - fsm princ\n");
                break;
        }
    }
    return SUCCESS;
}

int M() {
    xmlNode *curcomp = pointer;
    xmlNode *auxpointer = pointer;
    if (curcomp == NULL)
        return INVALID;

    while (curcomp != NULL) {
        // search for components in the file
        curcomp = get_component(curcomp);
        if (curcomp != NULL) {
            // search for math in the components
            xmlNode *curmath = get_math(curcomp);
            if (curmath != NULL) {
                math++;
                init_lex(curmath);
                if (C() != SUCCESS)
                    return INVALID;
            }
        }
    }
    if (math)
        return SUCCESS;
    else {
        init_lex(auxpointer);
        if (C() == SUCCESS)
            return SUCCESS;
        else
            return INVALID;
    }
}

int DV() {
    DiffHeader *diff = NULL;
    int estado = 0;
    Token token;
    memset(&token, 0, sizeof(Token));

    while (estado != 10) {
        switch (estado) {
            case 0: {
                // pode ser feito aqui nao existe expressao
                // que comece com apply, a nao ser a diff
                xmlNode *auxpointer = pointer;
                int auxi = lex_index;
                token = lex();
                if (token.type == OPAR)
                    estado = 1;
                else {
                    pointer = auxpointer;
                    lex_index = auxi;
                    return INVALID;
                }
            }
                break;
            case 1:
                token = lex();
                if (token.type == DIFF)
                    estado = 2;
                else
                    return INVALID;
                break;
            case 2:
                diff = (DiffHeader *) calloc(1, sizeof(DiffHeader));
                token = lex();

                if (token.type == VARI) {
                    token.isdiff = 1;
                    diff->diffvar = token;

                    estado = 3;
                } else {
                    if (token.type == BVAR) {
                        estado = 4;
                    } else
                        return INVALID;
                }
                break;
            case 3:
                token = lex();
                if (token.type == BVAR)
                    estado = 6;
                else
                    return INVALID;
                break;
            case 4:
                token = lex();
                if (token.type == VARI) {
                    diff->freevar = token;
                    estado = 5;
                } else
                    return INVALID;
                break;
            case 5:
                token = lex();
                if (token.type == CPAR)
                    estado = 7;
                else
                    return INVALID;
                break;
            case 6:
                token = lex();
                if (token.type == VARI) {
                    diff->freevar = token;
                    estado = 8;
                } else
                    return INVALID;
                break;
            case 7:

                token = lex();
                if (token.type == VARI) {
                    token.isdiff = 1;
                    diff->diffvar = token;

                    // concatenating the equal operator
                    estado = 9;
                } else
                    return INVALID;
                break;
            case 8:
                token = lex();
                if (token.type == CPAR)
                    estado = 9;
                else
                    return INVALID;
                break;
            case 9:

                token = lex();
                if (token.type == CPAR) {
                    eqpointer = concat_token(diff->diffvar, eqpointer);
                    if (!empty()) {
                        if (next(pointer) != NULL) {
                            eqpointer = concat_token(stack[s_ind], eqpointer);
                        }
                    }
                    estado = 10;
                } else
                    return INVALID;
                break;
            default:
                printf("ERROR - fsm P\n");
                break;
        }
    }

    return SUCCESS;
}

// Auxiliary implementation
void push(Token op) {
    s_ind++;
    stack[s_ind] = op;
}

Token pop() {
    if (empty()) {
        printf("%d: Compiler ERROR - Empty stack\n", pointer->line);
        exit(1);
    }
    Token op = stack[s_ind];
    s_ind--;
    return op;
}

int empty() {
    if (s_ind < 0)
        return 1;
    else
        return 0;
}

xmlNode *get_component(xmlNode *node) {
    xmlNode *cur = node;
    if (cur != NULL)
        if (!strcmp((char *) cur->name, "component"))
            cur = cur->next;
    while (cur != NULL) {
        if (!strcmp((char *) cur->name, "component"))
            return cur;
        if (!strcmp((char *) cur->name, "model"))
            cur = cur->children;
        else
            cur = cur->next;
    }
    return NULL;
}

xmlNode *get_math(xmlNode *node) {
    xmlNode *cur = node;
    while (cur != NULL) {
        if (!strcmp((char *) cur->name, "math"))
            return cur;
        if (!strcmp((char *) cur->name, "component"))
            cur = cur->children;
        else
            cur = cur->next;
    }
    return NULL;
}

xmlNode *get_variable(xmlNode *node) {
    xmlNode *cur = node;
    if (cur != NULL)
        if (!strcmp((char *) cur->name, "variable"))
            cur = cur->next;
    while (cur != NULL) {
        if (!strcmp((char *) cur->name, "variable"))
            return cur;
        if (!strcmp((char *) cur->name, "component"))
            cur = cur->children;
        else
            cur = cur->next;
    }
    return NULL;
}

void error(char *msg, xmlNode *errpointer) {
    if (errpointer != NULL) {
        printf("Line %d: ERROR - %s\n", errpointer->line, msg);
    }
}

#endif //_FSM_H_

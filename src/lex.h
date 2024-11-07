
#ifndef _LEX_H_
#define _LEX_H_

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include "fsm_util.h"



// TOKEN type definitions /////////////////////////////////////////////////////

#define    END        0    // end of file
#define    I_OP    1    // infix operator
#define    P_OP    2    // prefix operator
#define R_OP    12    // relational operator < > = <= >=
#define L_OP    16    // logical operator and or
#define PIOP    3    // mix operator only for '-' it may be unary
#define    NUMB    4    // number
#define    VARI    5    // variable
#define    DIFF    6    // diff operator
#define    BVAR    7    // bvar specification
#define OPAR    8    // open parenthesis (apply)
#define CPAR    9    // close parenthesis (#)
#define MATH    11    // math token
#define PI_W    13    // piecewise token
#define PIEC    14    // piece token
#define    OT_W    15    // otherwise token
#define ROOT    16    // root operator
#define DGRE    17    // used to especify root degree

#define RDF        18
#define MODL    19
#define COMP    20
#define VARB    21
#define UNIT    22


#define STACK_SIZE 50

// GLOBAL VARS ////////////////////////////////////////////////////////////////
xmlNode *pointer;

int lex_index = -1;
int lex_index_bound = STACK_SIZE;
xmlNode **lex_stack = NULL;


// FUNCTIONS PROTOTYPES ///////////////////////////////////////////////////////
void init_lex(xmlNode *node);

Token lex();

xmlNode *next(xmlNode *ptr);

Token get_token(xmlNode *ptr);

void lex_push(xmlNode *ptr);

xmlNode *lex_pop();

int lex_empty();

char *strNoSpace(char *str);

// FUNCTIONS IMPLEMENTATION ///////////////////////////////////////////////////

/* init_lex *******************************************************************
 * Initializes the stack and the xmlNode pointer with root node of the        *
 * document to be parsed                                                      *
 * ***************************************************************************/
void init_lex(xmlNode *node) {
    if (lex_stack != NULL)
        free(lex_stack);
    lex_stack = (xmlNode **) calloc(STACK_SIZE, sizeof(xmlNode *));
    pointer = node;
}


/* lex ************************************************************************
 * Returns the next token in the DOM tree                                     *
 * this function sets the token values                                        *
 *****************************************************************************/

Token lex() {
    Token token;
    token.tag = NULL;

    pointer = next(pointer);
    // if the pointer is NULL try to pop the stack
    if (pointer == NULL) {
        if (!lex_empty()) {
            pointer = lex_pop()->next;
            token.tag = (char *) "#";
            token.type = CPAR;
            token.content = (char *) ")";
        } else {
            token.tag = (char *) "END";
            token.type = END;
        }
    } else {
        while (pointer->type == XML_TEXT_NODE) {
            pointer = pointer->next;
            if (pointer == NULL)
                break;
        }
        if (pointer != NULL) {

            token = get_token(pointer);

            if (pointer->children != NULL) {
                // only push to stack these tags
                if (!strcmp((char *) pointer->name, "apply") ||
                    !strcmp((char *) pointer->name, "math") ||
                    !strcmp((char *) pointer->name, "bvar") ||
                    !strcmp((char *) pointer->name, "piecewise") ||
                    !strcmp((char *) pointer->name, "piece") ||
                    !strcmp((char *) pointer->name, "degree") ||
                    !strcmp((char *) pointer->name, "model") ||
                    !strcmp((char *) pointer->name, "component") ||
                    !strcmp((char *) pointer->name, "otherwise")) {
                    lex_push(pointer);
                    pointer = pointer->children;
                } else {
                    pointer = pointer->next;
                }
            } else {
                pointer = pointer->next;
            }
        }
    }

    return token;
}


/* get_token ******************************************************************
 * Translate node into token information                                      *
 * ***************************************************************************/

Token get_token(xmlNode *ptr) {
    Token token;
    memset(&token, 0, sizeof(Token));

    if (!strcmp((char *) ptr->name, "RDF")) {
        token.tag = (char *) ptr->name;
        token.content = token.tag;
        token.type = RDF;
    } else if (!strcmp((char *) ptr->name, "model")) {
        token.tag = (char *) ptr->name;
        token.content = token.tag;
        token.type = MODL;
    } else if (!strcmp((char *) ptr->name, "units")) {
        token.tag = (char *) ptr->name;
        token.content = token.tag;
        token.type = UNIT;
    } else if (!strcmp((char *) ptr->name, "component")) {
        token.tag = (char *) ptr->name;
        token.content = token.tag;
        token.type = COMP;

    } else if (!strcmp((char *) ptr->name, "variable")) {
        token.tag = (char *) ptr->name;
        token.content = token.tag;
        token.type = VARB;

    } else if (!strcmp((char *) ptr->name, "diff")) {
        token.tag = (char *) ptr->name;
        token.type = DIFF;

    } else if (!strcmp((char *) ptr->name, "apply")) {
        token.content = (char *) "(";
        token.tag = (char *) ptr->name;
        token.type = OPAR;
    } else if (!strcmp((char *) ptr->name, "plus")) {
        token.content = (char *) "+";
        token.tag = (char *) ptr->name;
        token.type = I_OP;
    } else if (!strcmp((char *) ptr->name, "divide") || !strcmp((char *) ptr->name, "quotient")) {
        token.content = (char *) "/";
        token.tag = (char *) ptr->name;
        token.type = I_OP;
    } else if (!strcmp((char *) ptr->name, "times")) {
        token.content = (char *) "*";
        token.tag = (char *) ptr->name;
        token.type = I_OP;
    } else if (!strcmp((char *) ptr->name, "minus")) {
        token.content = (char *) "-";
        token.tag = (char *) ptr->name;
        token.type = PIOP;
    } else if (!strcmp((char *) ptr->name, "rem")) {
        token.content = "%";
        token.tag = (char *) ptr->name;
        token.type = I_OP;
    } else if (!strcmp((char *) ptr->name, "math")) {
        token.tag = (char *) ptr->name;
        token.type = MATH;
    } else if (!strcmp((char *) ptr->name, "power")) {
        token.content = "pow(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "min")) {
        token.content = "fmin(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "max")) {
        token.content = "fmax(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "exp")) {
        token.content = (char *) "exp(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "floor")) {
        token.content = (char *) "floor(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "xor")) {
        token.content = (char *) "__agos_xor(";
        token.tag = (char *) ptr->name;
        token.type = L_OP;
    } else if (!strcmp((char *) ptr->name, "ln")) {
        token.content = (char *) "log(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "log")) {
        token.content = (char *) "log10(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "sin")) {
        token.content = (char *) "sin(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "cos")) {
        token.content = (char *) "cos(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "tan")) {
        token.content = (char *) "tan(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "csc")) {
        token.content = (char *) "1/sin(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "sec")) {
        token.content = (char *) "1/cos(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "cot")) {
        token.content = (char *) "1/tan(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "sinh")) {
        token.content = (char *) "sinh(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "cosh")) {
        token.content = (char *) "cosh(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "tanh")) {
        token.content = (char *) "tanh(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "csch")) {
        token.content = (char *) "1/sinh(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "sech")) {
        token.content = (char *) "1/cosh(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "coth")) {
        token.content = (char *) "1/tanh(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "arcsin")) {
        token.content = (char *) "asin(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "arccos")) {
        token.content = (char *) "acos(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "arctan")) {
        token.content = (char *) "atanf(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "arccsc")) {
        token.content = (char *) "1/asin(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "arcsec")) {
        token.content = (char *) "1/acos(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "arccot")) {
        token.content = (char *) "1/atan(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "arcsinh")) {
        token.content = (char *) "asinh(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "arccosh")) {
        token.content = (char *) "acosh(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "arctanh")) {
        token.content = (char *) "atanh(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "arccsch")) {
        token.content = (char *) "1/asinh(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "arcsech")) {
        token.content = (char *) "1/acosh(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "arccoth")) {
        token.content = (char *) "1/atanh(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "ceiling")) {
        token.content = (char *) "ceil(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "abs")) {
        token.content = (char *) "fabs(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    }
    if (!strcmp((char *) ptr->name, "eq")) {
        token.tag = (char *) ptr->name;
        token.content = (char *) "=";
        token.type = R_OP;
    } else if (!strcmp((char *) ptr->name, "ci")) {
        token.tag = (char *) ptr->name;
        token.content = strNoSpace((char *) xmlNodeGetContent(ptr));
        token.type = VARI;
        varlist = add_list(token, varlist);
    } else if (!strcmp((char *) ptr->name, "cn")) {
        token.tag = (char *) ptr->name;
        char *node_content;

        bool e_notation = false;

        xmlNode *cur = ptr->children;
        if((!xmlStrcmp( xmlGetProp(ptr, (const xmlChar*)"type"), (const xmlChar *)"e-notation"))) {
            node_content = (char *) xmlNodeGetContent(cur);
            e_notation = true;
            while (cur != NULL) {
                if ((!xmlStrcmp(cur->name, (const xmlChar *) "sep"))) {
                    cur = cur->next;
                    break;
                }
                cur = cur->next;
            }

        }
        else {
            node_content = (char*)xmlNodeGetContent(ptr);
        }

        double teste = strtod(node_content, NULL);
        char strteste[255];
        if(!e_notation)
            sprintf(strteste, "%.15e", teste);
        else {
            if(cur && cur->content) {
                long exp = strtol((char*)cur->content, NULL, 10);
                if(exp >= 0)
                    sprintf(strteste, "%lfe+0%ld", teste, exp);
                else
                    sprintf(strteste, "%lfe-0%ld", teste, labs(exp));
            }
            else {
                sprintf(strteste, "%.15e", teste);
            }
        }
        token.content = strNoSpace((char *) strteste);
        token.type = NUMB;
    }
        // CONSTANTS //////////////////////////////////////////////////////////
    else if (!strcmp((char *) ptr->name, "pi")) {
        token.tag = (char *) ptr->name;
        token.content = (char *) "3.141592653589793116";
        token.type = NUMB;
    } else if (!strcmp((char *) ptr->name, "exponentiale")) {
        token.tag = (char *) ptr->name;
        token.content = (char *) "2.718281828459045091";
        token.type = NUMB;
    } else if (!strcmp((char *) ptr->name, "true")) {
        token.tag = (char *) ptr->name;
        token.content = (char *) "1";
        token.type = NUMB;
    } else if (!strcmp((char *) ptr->name, "false")) {
        token.tag = (char *) ptr->name;
        token.content = (char *) "0";
        token.type = NUMB;
    } else if (!strcmp((char *) ptr->name, "infinity")) {
        token.tag = (char *) ptr->name;
        token.content = (char *) "AGOS_INF";
        token.type = NUMB;
    } else if (!strcmp((char *) ptr->name, "notanumber")) {
        token.tag = (char *) ptr->name;
        token.content = (char *) "AGOS_NAN";
        token.type = NUMB;
    }
        ///////////////////////////////////////////////////////////////////////
    else if (!strcmp((char *) ptr->name, "bvar")) {
        token.tag = (char *) ptr->name;
        token.content = NULL;
        token.type = BVAR;
    } else if (!strcmp((char *) ptr->name, "piecewise")) {
        token.tag = (char *) ptr->name;
        token.content = NULL;
        token.type = PI_W;
    } else if (!strcmp((char *) ptr->name, "piece")) {
        token.tag = (char *) ptr->name;
        token.content = NULL;
        token.type = PIEC;
    } else if (!strcmp((char *) ptr->name, "otherwise")) {
        token.tag = (char *) ptr->name;
        token.content = NULL;
        token.type = OT_W;
    } else if (!strcmp((char *) ptr->name, "gt")) {
        token.tag = (char *) ptr->name;
        token.content = (char *) ">";
        token.type = R_OP;
    } else if (!strcmp((char *) ptr->name, "lt")) {
        token.tag = (char *) ptr->name;
        token.content = (char *) "<";
        token.type = R_OP;
    } else if (!strcmp((char *) ptr->name, "geq")) {
        token.content = (char *) ">=";
        token.tag = (char *) ptr->name;
        token.type = R_OP;
    } else if (!strcmp((char *) ptr->name, "leq")) {
        token.content = (char *) "<=";
        token.tag = (char *) ptr->name;
        token.type = R_OP;
    } else if (!strcmp((char *) ptr->name, "neq")) {
        token.content = (char *) "!=";
        token.tag = (char *) ptr->name;
        token.type = R_OP;
    } else if (!strcmp((char *) ptr->name, "and")) {
        token.tag = (char *) ptr->name;
        token.content = (char *) "&&";
        token.type = L_OP;
    } else if (!strcmp((char *) ptr->name, "or")) {
        token.tag = (char *) ptr->name;
        token.content = (char *) "||";
        token.type = L_OP;
    } else if (!strcmp((char *) ptr->name, "root")) {
        token.content = (char *) "pow(";
        token.tag = (char *) ptr->name;
        token.type = ROOT;
    } else if (!strcmp((char *) ptr->name, "not")) {
        token.content = (char *) "!(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    } else if (!strcmp((char *) ptr->name, "degree")) {
        token.content = (char *) "";
        token.tag = (char *) ptr->name;
        token.type = DGRE;
    } else if (!strcmp((char *) ptr->name, "factorial")) {
        token.content = (char *) "__agos_factorial(";
        token.tag = (char *) ptr->name;
        token.type = P_OP;
    }

    return token;
}
/** Arithmetic_algebra *****************************************************
*   quotient, divide, plus, minus, times, rem. and, or, not                *
***************************************************************************/
/*
else if(!xmlStrcmp(name,(const xmlChar *)"not"))
{
	mmlnode->name = "!";     mmlnode->type = prefix;
}
*/
/** Relations **************************************************************
*   eq, neq, gt, lt, geq, leq                                              *
***************************************************************************/

/** Functions **************************************************************
*  exp, ln, log, sin, cos, tan, sec, csc, cot, sinh, cosh, tanh, sech,     *
*  csch, coth, arcsin, arccos, arctan, arcsec, arccsc, arccot, arcsinh,    *
*  arccosh, arctanh, arcsech, arccsch, arccoth,                            *
***************************************************************************/

void lex_push(xmlNode *ptr) {
    if (lex_index >= lex_index_bound) {
        lex_index_bound += STACK_SIZE / 2;
        lex_stack = (xmlNode **) realloc(lex_stack, sizeof(xmlNode *) * lex_index_bound);
        if (lex_stack == NULL) {
            printf("ERROR - lex stack reallocating memory failed\n");
            exit(1);
        }
    }
    lex_index++;
    lex_stack[lex_index] = ptr;
}

xmlNode *lex_pop() {
    xmlNode *ptr = NULL;
    if (lex_index >= 0) {
        ptr = lex_stack[lex_index];
        lex_index--;
    }
    return ptr;
}

int lex_empty() {
    if (lex_index >= 0)
        return 0;
    else return 1;
}

xmlNode *next(xmlNode *ptr) {
    xmlNode *cur = ptr;
    if (cur != NULL)
        while (!strcmp((char *) cur->name, "text") || !strcmp((char *) cur->name, "comment")) {
            cur = cur->next;
            if (cur == NULL)
                break;
        }
    return cur;
}

/* strNoSpace *****************************************************************
 * Returns a new string containing no spaces                                  *
 *****************************************************************************/

char *strNoSpace(char *str) {
    size_t size = strlen(str);
    int space = 0;

    size_t i;

    for (i = 0; i < size; i++)
        if (str[i] == ' ')
            space++;

    char *strn = (char *) calloc(size - space + 1, sizeof(char));

    space = 0;

    for (i = 0; i < size; i++) {
        if (str[i] != ' ') {
            strn[space] = str[i];
            space++;
        }
    }

    return strn;

}

#endif

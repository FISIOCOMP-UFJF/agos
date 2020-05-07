#ifndef _FSM_UTIL_H_
#define _FSM_UTIL_H_

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// gambiarra feita em 25/03/2008 por Ronan /////
// para nao ter que incluir o lex.h. Para resolver seria bom
// criar um .cpp de cada .h
#define PI_W	13	// piecewise token
////////////////////////////////////////////////

// STRUCTS ////////////////////////////////////////////////////////////////////
typedef struct _token_ {
    int		type;
    char	*tag;
    char	*content;
    int		isdiff;
} Token;

// struct that constructs a Token list for the equations
typedef struct node_ {
    Token token;
    char	*units;
    double	initialvalue;
    struct node_ *next;
    struct node_ *prev;
} TokenNode;

// struct that holds the differential equation header
typedef struct c_diff_ {
    Token diffvar;		// differential variable
    Token freevar;		// free variable
    TokenNode *eq;		// pointer to the equation
}DiffHeader;

// struct that holds the piecewise (IF) header
typedef struct if_eq_ {
    TokenNode *cond;	// relation equation that defines the if condicao
    TokenNode *piece;	// if true equation
    TokenNode *other;	// else equation
    int	if_counter;	// contador necessario para o caso de mais de um piece em um piecewise
}IfHeader;

// Diff Equations list
typedef struct diff_list {
    DiffHeader *diffheader;			// diff equation pointer
    struct diff_list  *next;		// next diff equation in the list
    struct diff_list  *prev;		// previous diff equation in the list
    int number;						// to put a index on equation
} DiffList;

// Algebraic Equations list
typedef struct algebraic_list {
    TokenNode *eq;					// algebraic equation pointer;
    struct algebraic_list *next;	// next algebraic equation in the list
    struct algebraic_list *prev;	// previous algebraic equation in the list
    int number;						// to put a index on equation
}AlgList;

typedef struct if_list {
    IfHeader *ifheader;				// if equation pointer
    struct if_list *next;			// next if equation in the list
    struct if_list *prev;			// previous if equation in the list
}IfList;


// GLOBALS ////////////////////////////////////////////////////////////////////
TokenNode 	*eqpointer = NULL;	// global pointer for equations
DiffList 	*difflist = NULL;	// list of differential equations
AlgList 	*alglist = NULL;	// list of algebraic equations
IfList 		*iflist = NULL;		// list of ifs

// TESTE - 08/03/2008 lista de variaveis das equacoes algebricas para a precedencia

AlgList		*preced_alg_list = NULL;


TokenNode 	*varlist = NULL;		// holds all vars in the equations
TokenNode	*difvarlist = NULL;		// list of diff vars
TokenNode	*algvarlist = NULL;		// list of algebraic vars
TokenNode	*parvarlist = NULL;		// list of parameters

int eq_counter = 0;		// to put index on equations
int p_if_counter = -1;		// to count ifs from different piecewises (when a piecewise have more than one if)

// PROTOTYPES /////////////////////////////////////////////////////////////////
TokenNode*	add_list(Token t, TokenNode *list);
TokenNode* 	attach_token(Token t, TokenNode *eq);
TokenNode* 	concat_token(Token t, TokenNode *eq);
DiffList* 	attach_diff_eq(DiffHeader *h, DiffList *list);
AlgList* 	attach_alg_eq(TokenNode *t, AlgList *list);
IfList*		attach_if_eq(IfHeader *h, IfList *list);
DiffList*	rewind_diff_list(DiffList *list);
AlgList*	rewind_alg_list(AlgList *list);
IfList*		rewind__if_list(IfList *list);
TokenNode*	rewind_token_list(TokenNode *list);
int 		list_has_var(Token t, TokenNode *list);
TokenNode*	create_param_list(TokenNode *avlist,TokenNode *dlist, TokenNode *alist);
int 		get_list_size(TokenNode *list);

AlgList*	create_preced_alg_list(AlgList *list);
AlgList *delete_from_list(AlgList *list, Token t);	// deleta o elemento atual da lista
bool	is_list_equal(TokenNode* list1, TokenNode *list2);


// IMPLEMENTATION /////////////////////////////////////////////////////////////


/* concat_token ***************************************************************
 * Concatenates a token at the end of the specified token list if the list    *
 * don't have the token                                                       *
 * ***************************************************************************/

TokenNode* add_list(Token t, TokenNode *list)
{
    if(!list_has_var(t, list))
    {
        return concat_token(t, list);
    }
    else return list;
}

/* attach_token ***************************************************************
 * Includes a token in the specified list as the next token of the current    *
 * token in the list                                                          *
 * ***************************************************************************/
TokenNode* attach_token(Token t, TokenNode *eq)
{
    TokenNode *current = (TokenNode*)calloc(1,sizeof(TokenNode));
    if(current == NULL)
    {
        printf("ERROR - allocating memory for TokenNode\n");
        exit(1);
    }
    current->token = t;
    if(eq != NULL)
    {
        current->prev = eq;
        current->next = eq->next;
        eq->next = current;
    }
    return current;
}

/* concat_token ***************************************************************
 * Concatenates a token at the end of the specified token list                *
 * ***************************************************************************/

TokenNode* concat_token(Token t, TokenNode *eq)
{
    TokenNode *aux;
    TokenNode *current = (TokenNode*)calloc(1,sizeof(TokenNode));
    if(current == NULL)
    {
        printf("ERROR - allocating memory for TokenNode\n");
        exit(1);
    }
    current->token = t;
    aux = eq;
    if(aux != NULL)
    {
        while(aux->next != NULL)
        {
            aux = eq->next;
        }
        current->prev = aux;
        aux->next = current;
    }
    return current;
}


/* attach_diff_eq *************************************************************
 * Includes a DiffList node in the list                                       *
 * ***************************************************************************/

DiffList* attach_diff_eq(DiffHeader *h, DiffList *list)
{
    DiffList *l = (DiffList*)calloc(1,sizeof(DiffList));
    if(!l){
        printf("ERROR - Allocating DiffList memory\n");
        exit(1);
    }
    l->diffheader = h;
    l->number = eq_counter;
    if(list != NULL){
        l->next = list->next;
        l->prev = list;
        list->next = l;
    }
    eq_counter++;
    return l;
}

/* attach_alg_eq *************************************************************
 * Includes a AlgList node in the list                                       *
 * **************************************************************************/

AlgList* attach_alg_eq(TokenNode *t, AlgList *list)
{

    AlgList *l = (AlgList*)calloc(1,sizeof(AlgList));
    if(!l){
        printf("ERROR - Allocating AlgList memory\n");
        exit(1);
    }
    l->eq = t;
    l->number = eq_counter;
    if(list != NULL){
        l->next = list->next;
        l->prev = list;
        list->next = l;
    }
    eq_counter++;
    return l;
}

/* attach_alg_eq *************************************************************
 * Includes a IfgList node in the list                                       *
 * **************************************************************************/

IfList*	attach_if_eq(IfHeader *h, IfList *list)
{
    IfList *l = (IfList*)calloc(1,sizeof(IfList));
    if(!l){
        printf("ERROR - Allocating IfList memory\n");
        exit(1);
    }
    l->ifheader = h;
    if(list != NULL){
        l->next = list->next;
        l->prev = list;
        list->next = l;
    }
    return l;
}

/* rewind_list ****************************************************************
 * Returns the first element in a DiffList                                    *
 * ***************************************************************************/

DiffList* rewind_diff_list(DiffList *list)
{
    DiffList *cur = list;
    if(cur != NULL)
        while(cur->prev != NULL)
            cur = cur->prev;
    return cur;
}

/* rewind_list ****************************************************************
 * Returns the first element in a AlgList                                     *
 * ***************************************************************************/

AlgList* rewind_alg_list(AlgList *list)
{
    AlgList *cur = list;
    if(cur != NULL)
        while(cur->prev != NULL)
            cur = cur->prev;
    return cur;
}

/* rewind_list ****************************************************************
 * Returns the first element in a IfList                                      *
 * ***************************************************************************/

IfList* rewind_if_list(IfList *list)
{
    IfList *cur = list;
    if(cur != NULL)
        while(cur->prev != NULL)
            cur = cur->prev;
    return cur;
}

/* rewind_list ****************************************************************
 * Returns the first element in a TokenList                                   *
 * ***************************************************************************/
//TODO: make a linked list with a descriptor
TokenNode* rewind_token_list(TokenNode *list)
{
    TokenNode *cur = list;
    if(cur != NULL) {
        while (cur->prev != NULL)
            cur = cur->prev;
    }
    return cur;
}


/* list_has_var ***************************************************************
 * Returns 1 if there's a var with the same name in the specified list        *
 * ***************************************************************************/

int list_has_var(Token t, TokenNode *list)
{
    TokenNode *cur = rewind_token_list(list);
    while(cur != NULL)
    {
        if(!strcmp(t.content, cur->token.content)) {
            return 1;
        }
        cur = cur->next;
    }
    return 0;
}


/* create_param_list **********************************************************
 * Returns a list created by (all var list) - (diff list) U (alg list)        *
 *****************************************************************************/

TokenNode*	create_param_list(TokenNode *avlist,TokenNode *dlist, TokenNode *alist)
{
    TokenNode *cur = rewind_token_list(avlist);
    TokenNode *plist = NULL;
    // putting the free var at first place in the param list
    plist = add_list(difflist->diffheader->freevar, plist);
    while(cur != NULL)
    {
        if(!list_has_var(cur->token, dlist) && !list_has_var(cur->token,alist))
        {
            if(!list_has_var(cur->token, plist)) {
                plist = add_list(cur->token, plist);
            }
        }
        cur = cur->next;
    }
    return plist;
}

int get_list_size(TokenNode *list)
{
    TokenNode *cur = rewind_token_list(list);
    int count = 0;
    while(cur != NULL)
    {
        count++;
        cur = cur->next;
    }
    return count;

}

// TESTE - 08/03/2008

// deve ser chamada depois de todas as listas serem criadas
AlgList* create_preced_alg_list(AlgList *list) {

    AlgList *cur = rewind_alg_list(list);
    AlgList *preced_list = NULL;
    if(cur != NULL)
        preced_list = (AlgList*)calloc(1,sizeof(AlgList));

    while(cur != NULL){
        TokenNode *cur_eq = cur->eq;
        TokenNode *plist = NULL;
        while(cur_eq != NULL){
            if(list_has_var(cur_eq->token, algvarlist)){
                if(!list_has_var(cur_eq->token, plist)){
                    plist = add_list(cur_eq->token, plist);
                    //printf("%s ", cur_eq->token.content);
                }
            }
            // achou um IF tenta achar alguma precedencia dentro do IF
            // talvez seja melhor depois fazer toda essa funcao como recursiva por causa de ifs aninhados
            if(cur_eq->token.type == PI_W){
                //printf("%s \\%d\n", cur_eq->token.content, cur->number);
                // buscando o if atual na lista de ifs
                IfList *cur_if = rewind_if_list(iflist);
                while(cur_if != NULL && cur_if->ifheader->if_counter != atoi(cur_eq->token.content))
                    cur_if = cur_if->next;

                TokenNode *cur_eq_aux = cur_eq;

                cur_eq = cur_if->ifheader->cond;
                while(cur_eq != NULL ){
                    if(list_has_var(cur_eq->token, algvarlist)){
                        if(!list_has_var(cur_eq->token, plist)){
                            plist = add_list(cur_eq->token, plist);
                            //printf("%s ", cur_eq->token.content);
                        }
                    }
                    cur_eq = cur_eq->next;
                }
                cur_eq = cur_if->ifheader->piece;
                while(cur_eq != NULL ){
                    if(list_has_var(cur_eq->token, algvarlist)){
                        if(!list_has_var(cur_eq->token, plist)){
                            plist = add_list(cur_eq->token, plist);
                            //printf("%s ", cur_eq->token.content);
                        }
                    }
                    cur_eq = cur_eq->next;
                }
                cur_eq = cur_if->ifheader->other;
                while(cur_eq != NULL ){
                    if(list_has_var(cur_eq->token, algvarlist)){
                        if(!list_has_var(cur_eq->token, plist)){
                            plist = add_list(cur_eq->token, plist);
                            //printf("%s ", cur_eq->token.content);
                        }
                    }
                    cur_eq = cur_eq->next;
                }

                cur_eq = cur_eq_aux;
            }
            cur_eq = cur_eq->next;
        }
        preced_list->eq = rewind_token_list(plist);
        cur = cur->next;
        if(cur != NULL){
            preced_list->next = (AlgList*)calloc(1,sizeof(AlgList));
            preced_list->next->prev = preced_list;
            preced_list = preced_list->next;

        }
        //printf("\n");
    }
    return rewind_alg_list(preced_list);
}

AlgList *delete_from_list(AlgList *list, Token t){
    AlgList *cur = list;
    while(cur != NULL){
        if(!strcmp(t.content, cur->eq->token.content)){
            if(cur->prev == NULL && cur->next == NULL){
                list = NULL;
            }
            else if(cur->prev == NULL){
                list = cur->next;
                list->prev = NULL;
            }else if(cur->next == NULL){
                cur->prev->next = NULL;
                cur = NULL;
            }else{
                cur->next->prev = cur->prev;
                cur->prev->next = cur->next;
            }
            break;
        }
        cur = cur->next;
    }

    return list;
    //list = cur;
}
///


#endif //_FSM_UTIL_H_

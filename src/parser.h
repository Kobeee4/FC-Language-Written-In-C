#ifndef FC_PARSER_H
#define FC_PARSER_H

#include <setjmp.h>
#include "lexer.h"

typedef enum {
    N_NUMBER, N_STRING, N_BOOL, N_NULL,
    N_LIST, N_DICT,
    N_IDENT, N_MEMBER, N_INDEX,
    N_BINARY, N_UNARY, N_CALL,
    N_ASSIGN, N_VARDECL, N_COMPOUND,
    N_OUT, N_INPUT, N_RETURN,
    N_FUNC, N_CLASS,
    N_IF, N_LOOP, N_WHILE, N_FOR,
    N_BREAK, N_CONTINUE,
    N_TRY, N_THREAD, N_IMPORT,
    N_BLOCK, N_EXPRSTMT
} NodeType;

typedef struct Node {
    NodeType type;
    int line;
    double number;
    int boolean;
    char* str;
    struct Node* a;
    struct Node* b;
    struct Node* c;
    struct Node** items;
    int item_count;
    char** params;
    int param_count;
} Node;

typedef struct {
    TokenList* tl;
    int pos;
    int had_error;
    char error_msg[256];
    int error_line;
    jmp_buf jb;
} Parser;

Node* parse_program(TokenList* tl, Parser* p);
void free_node(Node* n);

#endif

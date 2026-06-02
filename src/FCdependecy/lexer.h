#ifndef FC_LEXER_H
#define FC_LEXER_H

typedef enum {
    T_EOF, T_NEWLINE, T_INDENT, T_DEDENT,
    T_NUMBER, T_STRING, T_IDENT,
    T_LET, T_LISTKW, T_DICTKW, T_FUNC, T_RETURN, T_LOOP, T_TIMES,
    T_IF, T_ELIF, T_ELSE, T_WHILE, T_FOR, T_BREAK, T_CONTINUE,
    T_CLASS, T_TRY, T_CATCH,
    T_THREAD, T_SPAWN, T_INPUT, T_OUT, T_LAMBDA, T_IMPORT,
    T_TRUE, T_FALSE, T_NULL, T_AND, T_OR, T_NOT, T_IN,
    T_LPAREN, T_RPAREN, T_LBRACKET, T_RBRACKET, T_LBRACE, T_RBRACE,
    T_COMMA, T_COLON, T_DOT,
    T_PLUS, T_MINUS, T_STAR, T_SLASH, T_PERCENT,
    T_PLUSEQ, T_MINUSEQ, T_STAREQ, T_SLASHEQ,
    T_EQ, T_EQEQ, T_BANGEQ, T_LT, T_GT, T_LTE, T_GTE,
    T_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char* text;
    double number;
    int line;
} Token;

typedef struct {
    Token* tokens;
    int count;
    int had_error;
    char error_msg[256];
    int error_line;
} TokenList;

TokenList lex_source(const char* src);
void free_tokens(TokenList* tl);

#endif

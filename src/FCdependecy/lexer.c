#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

static char* dupn(const char* s, int n) {
    char* r = (char*)malloc(n + 1);
    memcpy(r, s, n);
    r[n] = 0;
    return r;
}

static void add_tok(TokenList* tl, int* cap, TokenType type, char* text, double num, int line) {
    if (tl->count >= *cap) {
        *cap = *cap ? *cap * 2 : 64;
        tl->tokens = (Token*)realloc(tl->tokens, (*cap) * sizeof(Token));
    }
    Token t;
    t.type = type;
    t.text = text;
    t.number = num;
    t.line = line;
    tl->tokens[tl->count++] = t;
}

static TokenType keyword_of(const char* s) {
    if (!strcmp(s, "let")) return T_LET;
    if (!strcmp(s, "list")) return T_LISTKW;
    if (!strcmp(s, "dict")) return T_DICTKW;
    if (!strcmp(s, "func")) return T_FUNC;
    if (!strcmp(s, "return")) return T_RETURN;
    if (!strcmp(s, "loop")) return T_LOOP;
    if (!strcmp(s, "times")) return T_TIMES;
    if (!strcmp(s, "if")) return T_IF;
    if (!strcmp(s, "elif")) return T_ELIF;
    if (!strcmp(s, "else")) return T_ELSE;
    if (!strcmp(s, "while")) return T_WHILE;
    if (!strcmp(s, "for")) return T_FOR;
    if (!strcmp(s, "break")) return T_BREAK;
    if (!strcmp(s, "continue")) return T_CONTINUE;
    if (!strcmp(s, "class")) return T_CLASS;
    if (!strcmp(s, "try")) return T_TRY;
    if (!strcmp(s, "catch")) return T_CATCH;
    if (!strcmp(s, "thread")) return T_THREAD;
    if (!strcmp(s, "spawn")) return T_SPAWN;
    if (!strcmp(s, "input")) return T_INPUT;
    if (!strcmp(s, "out")) return T_OUT;
    if (!strcmp(s, "lambda")) return T_LAMBDA;
    if (!strcmp(s, "import")) return T_IMPORT;
    if (!strcmp(s, "use")) return T_IMPORT;
    if (!strcmp(s, "true")) return T_TRUE;
    if (!strcmp(s, "false")) return T_FALSE;
    if (!strcmp(s, "null")) return T_NULL;
    if (!strcmp(s, "and")) return T_AND;
    if (!strcmp(s, "or")) return T_OR;
    if (!strcmp(s, "not")) return T_NOT;
    if (!strcmp(s, "in")) return T_IN;
    return T_IDENT;
}

static void fail(TokenList* tl, int line, const char* msg) {
    tl->had_error = 1;
    tl->error_line = line;
    strncpy(tl->error_msg, msg, sizeof(tl->error_msg) - 1);
}

TokenList lex_source(const char* src) {
    TokenList tl;
    tl.tokens = NULL;
    tl.count = 0;
    tl.had_error = 0;
    tl.error_msg[0] = 0;
    tl.error_line = 0;
    int cap = 0;

    int indent_stack[256];
    int indent_top = 0;
    indent_stack[0] = 0;

    const char* p = src;
    int line = 1;

    while (*p) {
        int col = 0;
        const char* q = p;
        while (*q == ' ' || *q == '\t') {
            col += (*q == '\t') ? 4 : 1;
            q++;
        }

        if (*q == '\n' || *q == '\r' || *q == 0) {
            p = q;
            if (*p == '\r') { p++; if (*p == '\n') p++; }
            else if (*p == '\n') { p++; }
            line++;
            continue;
        }

        if (*q == '+' && (q[1] == ' ' || q[1] == '\t' || q[1] == '\n' || q[1] == '\r' || q[1] == 0)) {
            p = q;
            while (*p && *p != '\n') p++;
            if (*p == '\n') p++;
            line++;
            continue;
        }

        if (col > indent_stack[indent_top]) {
            if (indent_top < 255) indent_stack[++indent_top] = col;
            add_tok(&tl, &cap, T_INDENT, NULL, 0, line);
        } else if (col < indent_stack[indent_top]) {
            while (indent_top > 0 && col < indent_stack[indent_top]) {
                indent_top--;
                add_tok(&tl, &cap, T_DEDENT, NULL, 0, line);
            }
            if (col != indent_stack[indent_top]) {
                fail(&tl, line, "Inconsistent indentation");
                return tl;
            }
        }

        p = q;
        while (*p && *p != '\n') {
            char c = *p;
            if (c == ' ' || c == '\t' || c == '\r') { p++; continue; }

            if (isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)p[1]))) {
                const char* s = p;
                while (isdigit((unsigned char)*p)) p++;
                if (*p == '.') { p++; while (isdigit((unsigned char)*p)) p++; }
                char* txt = dupn(s, (int)(p - s));
                add_tok(&tl, &cap, T_NUMBER, txt, strtod(txt, NULL), line);
                continue;
            }

            if (isalpha((unsigned char)c) || c == '_') {
                const char* s = p;
                while (isalnum((unsigned char)*p) || *p == '_') p++;
                char* txt = dupn(s, (int)(p - s));
                TokenType kt = keyword_of(txt);
                add_tok(&tl, &cap, kt, txt, 0, line);
                continue;
            }

            if (c == '"') {
                p++;
                int bcap = 16, blen = 0;
                char* buf = (char*)malloc(bcap);
                while (*p && *p != '"' && *p != '\n') {
                    char ch = *p;
                    if (ch == '\\') {
                        p++;
                        char e = *p;
                        switch (e) {
                            case 'n': ch = '\n'; break;
                            case 't': ch = '\t'; break;
                            case 'r': ch = '\r'; break;
                            case '\\': ch = '\\'; break;
                            case '"': ch = '"'; break;
                            case '0': ch = '\0'; break;
                            default: ch = e; break;
                        }
                        if (e == 0) break;
                    }
                    if (blen + 1 >= bcap) { bcap *= 2; buf = (char*)realloc(buf, bcap); }
                    buf[blen++] = ch;
                    p++;
                }
                if (*p != '"') {
                    free(buf);
                    fail(&tl, line, "Unterminated string literal");
                    return tl;
                }
                p++;
                buf[blen] = 0;
                add_tok(&tl, &cap, T_STRING, buf, 0, line);
                continue;
            }

            switch (c) {
                case '(': add_tok(&tl, &cap, T_LPAREN, NULL, 0, line); p++; break;
                case ')': add_tok(&tl, &cap, T_RPAREN, NULL, 0, line); p++; break;
                case '[': add_tok(&tl, &cap, T_LBRACKET, NULL, 0, line); p++; break;
                case ']': add_tok(&tl, &cap, T_RBRACKET, NULL, 0, line); p++; break;
                case '{': add_tok(&tl, &cap, T_LBRACE, NULL, 0, line); p++; break;
                case '}': add_tok(&tl, &cap, T_RBRACE, NULL, 0, line); p++; break;
                case ',': add_tok(&tl, &cap, T_COMMA, NULL, 0, line); p++; break;
                case ':': add_tok(&tl, &cap, T_COLON, NULL, 0, line); p++; break;
                case '.': add_tok(&tl, &cap, T_DOT, NULL, 0, line); p++; break;
                case '+':
                    if (p[1] == '=') { add_tok(&tl, &cap, T_PLUSEQ, NULL, 0, line); p += 2; }
                    else { add_tok(&tl, &cap, T_PLUS, NULL, 0, line); p++; }
                    break;
                case '-':
                    if (p[1] == '=') { add_tok(&tl, &cap, T_MINUSEQ, NULL, 0, line); p += 2; }
                    else { add_tok(&tl, &cap, T_MINUS, NULL, 0, line); p++; }
                    break;
                case '*':
                    if (p[1] == '=') { add_tok(&tl, &cap, T_STAREQ, NULL, 0, line); p += 2; }
                    else { add_tok(&tl, &cap, T_STAR, NULL, 0, line); p++; }
                    break;
                case '/':
                    if (p[1] == '=') { add_tok(&tl, &cap, T_SLASHEQ, NULL, 0, line); p += 2; }
                    else { add_tok(&tl, &cap, T_SLASH, NULL, 0, line); p++; }
                    break;
                case '%': add_tok(&tl, &cap, T_PERCENT, NULL, 0, line); p++; break;
                case '=':
                    if (p[1] == '=') { add_tok(&tl, &cap, T_EQEQ, NULL, 0, line); p += 2; }
                    else { add_tok(&tl, &cap, T_EQ, NULL, 0, line); p++; }
                    break;
                case '!':
                    if (p[1] == '=') { add_tok(&tl, &cap, T_BANGEQ, NULL, 0, line); p += 2; }
                    else { fail(&tl, line, "Unexpected character '!'"); return tl; }
                    break;
                case '<':
                    if (p[1] == '=') { add_tok(&tl, &cap, T_LTE, NULL, 0, line); p += 2; }
                    else { add_tok(&tl, &cap, T_LT, NULL, 0, line); p++; }
                    break;
                case '>':
                    if (p[1] == '=') { add_tok(&tl, &cap, T_GTE, NULL, 0, line); p += 2; }
                    else { add_tok(&tl, &cap, T_GT, NULL, 0, line); p++; }
                    break;
                default: {
                    char m[64];
                    snprintf(m, sizeof(m), "Unexpected character '%c'", c);
                    fail(&tl, line, m);
                    return tl;
                }
            }
        }

        add_tok(&tl, &cap, T_NEWLINE, NULL, 0, line);
        if (*p == '\n') p++;
        line++;
    }

    while (indent_top > 0) {
        indent_top--;
        add_tok(&tl, &cap, T_DEDENT, NULL, 0, line);
    }
    add_tok(&tl, &cap, T_EOF, NULL, 0, line);
    return tl;
}

void free_tokens(TokenList* tl) {
    if (!tl->tokens) return;
    for (int i = 0; i < tl->count; i++) {
        if (tl->tokens[i].text) free(tl->tokens[i].text);
    }
    free(tl->tokens);
    tl->tokens = NULL;
    tl->count = 0;
}

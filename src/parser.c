#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parser.h"

static Node* parse_statement(Parser* p);
static Node* parse_expr(Parser* p);
static Node* parse_block(Parser* p);
static Node* parse_postfix(Parser* p);

static Node* new_node(NodeType t, int line) {
    Node* n = (Node*)calloc(1, sizeof(Node));
    n->type = t;
    n->line = line;
    return n;
}

static void add_item(Node* n, Node* child) {
    n->items = (Node**)realloc(n->items, sizeof(Node*) * (n->item_count + 1));
    n->items[n->item_count++] = child;
}

static void add_param(Node* n, const char* name) {
    n->params = (char**)realloc(n->params, sizeof(char*) * (n->param_count + 1));
    n->params[n->param_count++] = strdup(name);
}

static Token* cur(Parser* p) { return &p->tl->tokens[p->pos]; }
static TokenType curt(Parser* p) { return p->tl->tokens[p->pos].type; }
static int curline(Parser* p) { return p->tl->tokens[p->pos].line; }
static Token* advance(Parser* p) { return &p->tl->tokens[p->pos++]; }
static int check(Parser* p, TokenType t) { return curt(p) == t; }

static int match(Parser* p, TokenType t) {
    if (check(p, t)) { p->pos++; return 1; }
    return 0;
}

static void perror_at(Parser* p, int line, const char* msg) {
    p->had_error = 1;
    p->error_line = line;
    strncpy(p->error_msg, msg, sizeof(p->error_msg) - 1);
    p->error_msg[sizeof(p->error_msg) - 1] = 0;
    longjmp(p->jb, 1);
}

static const char* tok_literal(TokenType t) {
    switch (t) {
        case T_LPAREN: return "(";
        case T_RPAREN: return ")";
        case T_LBRACKET: return "[";
        case T_RBRACKET: return "]";
        case T_LBRACE: return "{";
        case T_RBRACE: return "}";
        case T_COMMA: return ",";
        case T_COLON: return ":";
        case T_DOT: return ".";
        case T_EQ: return "=";
        case T_NEWLINE: return "end of line";
        case T_EOF: return "end of file";
        case T_INDENT: return "more indentation";
        case T_DEDENT: return "less indentation";
        default: return NULL;
    }
}

static const char* tok_desc(Parser* p) {
    static char buf[80];
    Token* t = &p->tl->tokens[p->pos];
    if (t->type == T_NUMBER) { snprintf(buf, sizeof(buf), "number '%s'", t->text ? t->text : "?"); return buf; }
    if (t->type == T_STRING) return "a string";
    if (t->type == T_IDENT) { snprintf(buf, sizeof(buf), "name '%s'", t->text ? t->text : "?"); return buf; }
    if (t->text) { snprintf(buf, sizeof(buf), "keyword '%s'", t->text); return buf; }
    const char* lit = tok_literal(t->type);
    if (lit) { snprintf(buf, sizeof(buf), "'%s'", lit); return buf; }
    return "this token";
}

static Token* expect(Parser* p, TokenType t, const char* msg) {
    if (check(p, t)) return advance(p);
    char full[256];
    snprintf(full, sizeof(full), "%s, but found %s", msg, tok_desc(p));
    perror_at(p, curline(p), full);
    return NULL;
}

static void skip_newlines(Parser* p) {
    while (check(p, T_NEWLINE)) advance(p);
}

static const char* op_text(TokenType t) {
    switch (t) {
        case T_PLUS: return "+";
        case T_MINUS: return "-";
        case T_STAR: return "*";
        case T_SLASH: return "/";
        case T_PERCENT: return "%";
        case T_EQEQ: return "==";
        case T_BANGEQ: return "!=";
        case T_LT: return "<";
        case T_GT: return ">";
        case T_LTE: return "<=";
        case T_GTE: return ">=";
        case T_AND: return "and";
        case T_OR: return "or";
        case T_NOT: return "not";
        case T_IN: return "in";
        default: return "?";
    }
}

static Node* parse_primary(Parser* p) {
    int line = curline(p);
    if (check(p, T_LAMBDA)) {
        advance(p);
        Node* n = new_node(N_FUNC, line);
        n->str = strdup("lambda");
        if (!check(p, T_COLON)) {
            do {
                Token* pn = expect(p, T_IDENT, "Expected lambda parameter name");
                add_param(n, pn->text);
            } while (match(p, T_COMMA));
        }
        expect(p, T_COLON, "Expected ':' in lambda");
        Node* body = new_node(N_BLOCK, line);
        Node* ret = new_node(N_RETURN, line);
        ret->a = parse_expr(p);
        add_item(body, ret);
        n->a = body;
        return n;
    }
    if (check(p, T_NUMBER)) {
        Node* n = new_node(N_NUMBER, line);
        n->number = cur(p)->number;
        advance(p);
        return n;
    }
    if (check(p, T_STRING)) {
        Node* n = new_node(N_STRING, line);
        n->str = strdup(cur(p)->text ? cur(p)->text : "");
        advance(p);
        return n;
    }
    if (check(p, T_TRUE)) { advance(p); Node* n = new_node(N_BOOL, line); n->boolean = 1; return n; }
    if (check(p, T_FALSE)) { advance(p); Node* n = new_node(N_BOOL, line); n->boolean = 0; return n; }
    if (check(p, T_NULL)) { advance(p); return new_node(N_NULL, line); }
    if (check(p, T_IDENT)) {
        Node* n = new_node(N_IDENT, line);
        n->str = strdup(cur(p)->text);
        advance(p);
        return n;
    }
    if (match(p, T_LPAREN)) {
        Node* e = parse_expr(p);
        expect(p, T_RPAREN, "Expected ')' after expression");
        return e;
    }
    if (check(p, T_LBRACKET)) {
        advance(p);
        Node* n = new_node(N_LIST, line);
        if (!check(p, T_RBRACKET)) {
            do {
                skip_newlines(p);
                add_item(n, parse_expr(p));
                skip_newlines(p);
            } while (match(p, T_COMMA));
        }
        expect(p, T_RBRACKET, "Expected ']' to close list");
        return n;
    }
    if (check(p, T_LBRACE)) {
        advance(p);
        Node* n = new_node(N_DICT, line);
        if (!check(p, T_RBRACE)) {
            do {
                skip_newlines(p);
                Node* k = parse_expr(p);
                expect(p, T_COLON, "Expected ':' in dict literal");
                Node* v = parse_expr(p);
                add_item(n, k);
                add_item(n, v);
                skip_newlines(p);
            } while (match(p, T_COMMA));
        }
        expect(p, T_RBRACE, "Expected '}' to close dict");
        return n;
    }
    {
        char full[256];
        snprintf(full, sizeof(full), "Expected a value or expression, but found %s", tok_desc(p));
        perror_at(p, line, full);
    }
    return NULL;
}

static Node* parse_postfix(Parser* p) {
    Node* e = parse_primary(p);
    for (;;) {
        int line = curline(p);
        if (match(p, T_LPAREN)) {
            Node* call = new_node(N_CALL, line);
            call->a = e;
            if (!check(p, T_RPAREN)) {
                do {
                    skip_newlines(p);
                    add_item(call, parse_expr(p));
                    skip_newlines(p);
                } while (match(p, T_COMMA));
            }
            expect(p, T_RPAREN, "Expected ')' after arguments");
            e = call;
        } else if (match(p, T_DOT)) {
            Token* name = cur(p);
            if (name->text == NULL || name->type == T_NUMBER || name->type == T_STRING)
                perror_at(p, line, "Expected member name after '.'");
            advance(p);
            Node* m = new_node(N_MEMBER, line);
            m->a = e;
            m->str = strdup(name->text);
            e = m;
        } else if (match(p, T_LBRACKET)) {
            Node* idx = new_node(N_INDEX, line);
            idx->a = e;
            idx->b = parse_expr(p);
            expect(p, T_RBRACKET, "Expected ']' after index");
            e = idx;
        } else {
            break;
        }
    }
    return e;
}

static Node* parse_unary(Parser* p) {
    int line = curline(p);
    if (check(p, T_NOT) || check(p, T_MINUS)) {
        TokenType t = curt(p);
        advance(p);
        Node* n = new_node(N_UNARY, line);
        n->str = strdup(op_text(t));
        n->a = parse_unary(p);
        return n;
    }
    return parse_postfix(p);
}

static Node* bin(Parser* p, Node* (*next)(Parser*), const TokenType* ops, int nops) {
    Node* left = next(p);
    for (;;) {
        int matched = 0;
        for (int i = 0; i < nops; i++) {
            if (check(p, ops[i])) {
                int line = curline(p);
                TokenType t = curt(p);
                advance(p);
                Node* n = new_node(N_BINARY, line);
                n->str = strdup(op_text(t));
                n->a = left;
                n->b = next(p);
                left = n;
                matched = 1;
                break;
            }
        }
        if (!matched) break;
    }
    return left;
}

static Node* parse_factor(Parser* p) {
    static const TokenType ops[] = { T_STAR, T_SLASH, T_PERCENT };
    return bin(p, parse_unary, ops, 3);
}
static Node* parse_term(Parser* p) {
    static const TokenType ops[] = { T_PLUS, T_MINUS };
    return bin(p, parse_factor, ops, 2);
}
static Node* parse_comparison(Parser* p) {
    static const TokenType ops[] = { T_LT, T_GT, T_LTE, T_GTE, T_IN };
    return bin(p, parse_term, ops, 5);
}
static Node* parse_equality(Parser* p) {
    static const TokenType ops[] = { T_EQEQ, T_BANGEQ };
    return bin(p, parse_comparison, ops, 2);
}
static Node* parse_and(Parser* p) {
    static const TokenType ops[] = { T_AND };
    return bin(p, parse_equality, ops, 1);
}
static Node* parse_or(Parser* p) {
    static const TokenType ops[] = { T_OR };
    return bin(p, parse_and, ops, 1);
}
static Node* parse_expr(Parser* p) {
    return parse_or(p);
}

static Node* parse_block(Parser* p) {
    expect(p, T_NEWLINE, "Expected newline before indented block");
    expect(p, T_INDENT, "Expected an indented block");
    Node* block = new_node(N_BLOCK, curline(p));
    while (!check(p, T_DEDENT) && !check(p, T_EOF)) {
        if (check(p, T_NEWLINE)) { advance(p); continue; }
        add_item(block, parse_statement(p));
    }
    match(p, T_DEDENT);
    return block;
}

static Node* parse_func(Parser* p) {
    int line = curline(p);
    expect(p, T_FUNC, "Expected 'func'");
    Token* name = expect(p, T_IDENT, "Expected function name");
    Node* n = new_node(N_FUNC, line);
    n->str = strdup(name->text);
    expect(p, T_LPAREN, "Expected '(' after function name");
    if (!check(p, T_RPAREN)) {
        do {
            Token* pn = expect(p, T_IDENT, "Expected parameter name");
            add_param(n, pn->text);
        } while (match(p, T_COMMA));
    }
    expect(p, T_RPAREN, "Expected ')' after parameters");
    n->a = parse_block(p);
    return n;
}

static Node* parse_class(Parser* p) {
    int line = curline(p);
    expect(p, T_CLASS, "Expected 'class'");
    Token* name = expect(p, T_IDENT, "Expected class name");
    Node* n = new_node(N_CLASS, line);
    n->str = strdup(name->text);
    expect(p, T_NEWLINE, "Expected newline after class header");
    expect(p, T_INDENT, "Expected indented class body");
    while (!check(p, T_DEDENT) && !check(p, T_EOF)) {
        if (check(p, T_NEWLINE)) { advance(p); continue; }
        if (check(p, T_FUNC)) {
            add_item(n, parse_func(p));
        } else {
            perror_at(p, curline(p), "Only methods are allowed inside a class body");
        }
    }
    match(p, T_DEDENT);
    return n;
}

static Node* parse_if(Parser* p) {
    int line = curline(p);
    advance(p);
    Node* n = new_node(N_IF, line);
    n->a = parse_expr(p);
    n->b = parse_block(p);
    if (check(p, T_ELIF)) {
        n->c = parse_if(p);
    } else if (match(p, T_ELSE)) {
        n->c = parse_block(p);
    }
    return n;
}

static int is_lvalue(Node* n) {
    return n->type == N_IDENT || n->type == N_MEMBER || n->type == N_INDEX;
}

static Node* parse_statement(Parser* p) {
    int line = curline(p);

    if (check(p, T_LET) || check(p, T_LISTKW) || check(p, T_DICTKW)) {
        advance(p);
        Node* target = parse_postfix(p);
        expect(p, T_EQ, "Expected '=' in declaration");
        Node* value = parse_expr(p);
        expect(p, T_NEWLINE, "Expected newline after declaration");
        if (target->type == N_IDENT) {
            Node* n = new_node(N_VARDECL, line);
            n->str = strdup(target->str);
            n->a = value;
            free_node(target);
            return n;
        }
        Node* n = new_node(N_ASSIGN, line);
        n->a = target;
        n->b = value;
        return n;
    }

    if (check(p, T_OUT)) {
        advance(p);
        Node* n = new_node(N_OUT, line);
        n->a = parse_expr(p);
        expect(p, T_NEWLINE, "Expected newline after 'out'");
        return n;
    }

    if (check(p, T_INPUT)) {
        advance(p);
        Token* name = expect(p, T_IDENT, "Expected variable name after 'input'");
        Node* n = new_node(N_INPUT, line);
        n->str = strdup(name->text);
        expect(p, T_NEWLINE, "Expected newline after 'input'");
        return n;
    }

    if (check(p, T_RETURN)) {
        advance(p);
        Node* n = new_node(N_RETURN, line);
        if (!check(p, T_NEWLINE)) n->a = parse_expr(p);
        expect(p, T_NEWLINE, "Expected newline after 'return'");
        return n;
    }

    if (check(p, T_FUNC)) return parse_func(p);
    if (check(p, T_CLASS)) return parse_class(p);
    if (check(p, T_IF)) return parse_if(p);

    if (check(p, T_LOOP)) {
        advance(p);
        Node* n = new_node(N_LOOP, line);
        n->a = parse_expr(p);
        expect(p, T_TIMES, "Expected 'times' after loop count");
        n->b = parse_block(p);
        return n;
    }

    if (check(p, T_WHILE)) {
        advance(p);
        Node* n = new_node(N_WHILE, line);
        n->a = parse_expr(p);
        n->b = parse_block(p);
        return n;
    }

    if (check(p, T_FOR)) {
        advance(p);
        Token* var = expect(p, T_IDENT, "Expected variable name after 'for'");
        Node* n = new_node(N_FOR, line);
        n->str = strdup(var->text);
        expect(p, T_IN, "Expected 'in' after for-loop variable");
        n->a = parse_expr(p);
        n->b = parse_block(p);
        return n;
    }

    if (check(p, T_BREAK)) {
        advance(p);
        expect(p, T_NEWLINE, "Expected newline after 'break'");
        return new_node(N_BREAK, line);
    }

    if (check(p, T_CONTINUE)) {
        advance(p);
        expect(p, T_NEWLINE, "Expected newline after 'continue'");
        return new_node(N_CONTINUE, line);
    }

    if (check(p, T_TRY)) {
        advance(p);
        Node* n = new_node(N_TRY, line);
        n->a = parse_block(p);
        expect(p, T_CATCH, "Expected 'catch' after try block");
        Token* var = expect(p, T_IDENT, "Expected error variable after 'catch'");
        n->str = strdup(var->text);
        n->b = parse_block(p);
        return n;
    }

    if (check(p, T_IMPORT)) {
        advance(p);
        Node* n = new_node(N_IMPORT, line);
        if (check(p, T_STRING) || check(p, T_IDENT)) {
            n->str = strdup(cur(p)->text);
            advance(p);
        } else {
            perror_at(p, line, "Expected a module name or path after 'import'");
        }
        expect(p, T_NEWLINE, "Expected newline after import");
        return n;
    }

    if (check(p, T_THREAD)) {
        advance(p);
        expect(p, T_SPAWN, "Expected 'spawn' after 'thread'");
        Node* n = new_node(N_THREAD, line);
        n->a = parse_expr(p);
        expect(p, T_NEWLINE, "Expected newline after thread spawn");
        return n;
    }

    Node* e = parse_postfix(p);
    if (check(p, T_EQ)) {
        if (!is_lvalue(e)) perror_at(p, line, "Invalid assignment target");
        advance(p);
        Node* n = new_node(N_ASSIGN, line);
        n->a = e;
        n->b = parse_expr(p);
        expect(p, T_NEWLINE, "Expected newline after assignment");
        return n;
    }
    if (check(p, T_PLUSEQ) || check(p, T_MINUSEQ) || check(p, T_STAREQ) || check(p, T_SLASHEQ)) {
        if (!is_lvalue(e)) perror_at(p, line, "Invalid assignment target");
        TokenType t = curt(p);
        advance(p);
        const char* op = t == T_PLUSEQ ? "+" : t == T_MINUSEQ ? "-" : t == T_STAREQ ? "*" : "/";
        Node* n = new_node(N_COMPOUND, line);
        n->a = e;
        n->str = strdup(op);
        n->b = parse_expr(p);
        expect(p, T_NEWLINE, "Expected newline after assignment");
        return n;
    }
    Node* stmt = new_node(N_EXPRSTMT, line);
    stmt->a = e;
    expect(p, T_NEWLINE, "Expected newline after expression");
    return stmt;
}

Node* parse_program(TokenList* tl, Parser* p) {
    p->tl = tl;
    p->pos = 0;
    p->had_error = 0;
    p->error_msg[0] = 0;
    p->error_line = 0;
    if (setjmp(p->jb)) {
        return NULL;
    }
    Node* block = new_node(N_BLOCK, 1);
    while (!check(p, T_EOF)) {
        if (check(p, T_NEWLINE) || check(p, T_INDENT) || check(p, T_DEDENT)) {
            advance(p);
            continue;
        }
        add_item(block, parse_statement(p));
    }
    return block;
}

void free_node(Node* n) {
    if (!n) return;
    if (n->str) free(n->str);
    if (n->a) free_node(n->a);
    if (n->b) free_node(n->b);
    if (n->c) free_node(n->c);
    for (int i = 0; i < n->item_count; i++) free_node(n->items[i]);
    if (n->items) free(n->items);
    for (int i = 0; i < n->param_count; i++) free(n->params[i]);
    if (n->params) free(n->params);
    free(n);
}

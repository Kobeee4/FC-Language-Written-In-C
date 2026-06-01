#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "interpreter.h"

static int run_source(Interp* I, const char* src, Node** out_prog) {
    *out_prog = NULL;
    TokenList tl = lex_source(src);
    if (tl.had_error) {
        fprintf(stderr, "FC Syntax Error on Line %d: %s\n", tl.error_line, tl.error_msg);
        free_tokens(&tl);
        return 1;
    }
    Parser p;
    Node* program = parse_program(&tl, &p);
    if (!program || p.had_error) {
        fprintf(stderr, "FC Parse Error on Line %d: %s\n", p.error_line, p.error_msg);
        if (program) free_node(program);
        free_tokens(&tl);
        return 1;
    }
    free_tokens(&tl);
    interp_run(I, program);
    int code = I->had_error ? 1 : 0;
    I->had_error = 0;
    *out_prog = program;
    return code;
}

static int run_file(const char* path, int argc, char** argv) {
    char* src = read_file(path);
    if (!src) {
        fprintf(stderr, "FC Error: cannot open file '%s'\n", path);
        return 1;
    }
    Interp* I = interp_new();
    I->argc = argc;
    I->argv = argv;
    register_modules(I);
    Node* prog = NULL;
    int code = run_source(I, src, &prog);
    if (prog) free_node(prog);
    interp_free(I);
    free(src);
    return code;
}

static int needs_more(const char* line) {
    const char* p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "func", 4) == 0) return 1;
    if (strncmp(p, "class", 5) == 0) return 1;
    if (strncmp(p, "if", 2) == 0) return 1;
    if (strncmp(p, "elif", 4) == 0) return 1;
    if (strncmp(p, "else", 4) == 0) return 1;
    if (strncmp(p, "loop", 4) == 0) return 1;
    if (strncmp(p, "while", 5) == 0) return 1;
    if (strncmp(p, "try", 3) == 0) return 1;
    if (strncmp(p, "catch", 5) == 0) return 1;
    return 0;
}

static int repl(void) {
    Interp* I = interp_new();
    register_modules(I);
    printf("FC Language REPL. Type code; a blank line runs a buffered block. Ctrl-D to exit.\n");

    Node** progs = NULL;
    int prog_count = 0, prog_cap = 0;

    char line[2048];
    int bcap = 4096, blen = 0;
    char* buffer = (char*)malloc(bcap);
    buffer[0] = 0;
    int in_block = 0;

    for (;;) {
        printf(in_block ? "... " : "fc> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        if (needs_more(line)) in_block = 1;
        int only_ws = 1;
        for (char* c = line; *c; c++) if (*c != ' ' && *c != '\t' && *c != '\n' && *c != '\r') { only_ws = 0; break; }
        int ll = (int)strlen(line);
        if (blen + ll + 1 > bcap) { while (blen + ll + 1 > bcap) bcap *= 2; buffer = (char*)realloc(buffer, bcap); }
        memcpy(buffer + blen, line, ll);
        blen += ll;
        buffer[blen] = 0;
        if (in_block && !only_ws) continue;
        if (in_block && only_ws) in_block = 0;

        Node* prog = NULL;
        run_source(I, buffer, &prog);
        if (prog) {
            if (prog_count >= prog_cap) { prog_cap = prog_cap ? prog_cap * 2 : 16; progs = (Node**)realloc(progs, sizeof(Node*) * prog_cap); }
            progs[prog_count++] = prog;
        }
        blen = 0;
        buffer[0] = 0;
    }

    free(buffer);
    interp_free(I);
    for (int i = 0; i < prog_count; i++) free_node(progs[i]);
    free(progs);
    printf("\n");
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 2) return run_file(argv[1], argc - 1, argv + 1);
    return repl();
}

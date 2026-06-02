# Architecture — FCL (FuseCore Language)

This document explains the main components of the FCL implementation and how they interact.

High level components

1. Lexer (src/lexer.c, src/lexer.h)
   - Tokenizes source text, handles indentation, numbers, strings, identifiers, and keywords.
   - Emits INDENT/DEDENT tokens so the parser can build block structure.

2. Parser (src/parser.c, src/parser.h)
   - Consumes tokens produced by the lexer and builds an AST (Node tree).
   - Implements expression parsing, statements, function/class definitions, control flow, and basic error handling.

3. AST (Node)
   - A compact tree representing expressions and statements (see parser.h Node structure).
   - Used by the interpreter to evaluate code or by future passes (bytecode, codegen).

4. Interpreter / Runtime (src/interpreter.c, src/main.c)
   - Walks the AST and evaluates code using a Value representation (numbers, strings, lists, dicts, functions).
   - Implements environment, call frames, built-in native functions, and a REPL.

5. Native modules (src/modules.c and related C files)
   - Native functions are registered in modules (math, ai, nn, file, web, crypto, sec, random, os, etc.).
   - Modules expose a C API to scripts via dicts mapping names to native functions.

6. Demos and scripts (src/fcdemo/*)
   - Example scripts that exercise modules (AI demo, math helpers, security, web, file operations).

Build and repo layout

- src/: main sources for the original project (contains the full interpreter build and a src/Makefile)
- bin/: produced by the top-level Makefile (convenience executable at bin/fcl)
- docs/: user and developer documentation
- top-level Makefile: convenience wrapper to build and place an executable at bin/fcl

Extending the project

- Adding a native module:
  1. Implement C functions using the `Value f(Interp*, Value, int, Value*)` signature
  2. Build a dict of functions and register with `register_my_mod(Interp* I)`
  3. Add the file to `SRCS` in src/Makefile and rebuild

Runtime data model

- Value: unified runtime value (number, string, list, dict, native function, user function)
- Lists and dicts are heap objects referenced by Values
- Memory ownership uses helper functions; the interpreter manages lifetime for returned values

Testing and CI

- The project currently uses manual builds. Adding a CI workflow that runs `make` on pushes and pull requests is recommended for automated build verification.

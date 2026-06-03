<p align="center">
  <img src="images/fc_logo.png" width="220">
</p>

<h1 align="center">FC Language</h1>

<p align="center">
Experimental programming language written in C
</p>

# FCL (FuseCore Language) - written in C

A small but real programming language implementation written in C. This repository contains a working lexer, parser, interpreter, and a set of native modules, including AI and neural-network helpers. The project is maintained solo by the author, who develops primarily from a mobile device. with a little help from claude code

Author: Kobeee4

## Note, This MD was created by AI. 
Overview

FCL (FuseCore Language) is an experimental, lightweight language implemented in C. It is intended for learning, experimentation, and quick demos. The repository includes a lexer, a parser, an AST, a runtime/interpreter, and a collection of native modules exposed to scripts.

Repository contents (high level)

- src/: core interpreter sources (lexer, parser, interpreter, modules, demos)
- docs/: user and developer-oriented documentation
- bin/: produced by the top-level Makefile (convenience executable at bin/fcl)
- LICENSE: MIT license

Key components

- Lexer: src/lexer.c, src/lexer.h
- Parser: src/parser.c, src/parser.h
- Interpreter and REPL: src/interpreter.c, src/main.c
- Native modules: src/modules.c (ai, nn, math, crypto, web, file, os, random, sec, etc.)
- Demos and scripts: src/fcdemo (including math.ai demo) and src/update (small example and a stub runtime)

Build and run

Two build options are available depending on how you prefer to work.

1) Top-level convenience build (produces ./bin/fcl)

From the repository root:

```sh
make
```

Then run a script with:

```sh
./bin/fcl path/to/script.fc
```

2) Original src layout build (produces ./src/fc)

```sh
cd src
make
./fc path/to/script.fc
```

Examples

- AI + NN demo (matrix ops and tiny neural forward pass):

```sh
./bin/fcl src/fcdemo/math.fc
```

- Quick example (stub runtime that prints the file contents):

```sh
./bin/fcl src/update/example.fc
```

Mobile / Termux notes

Termux provides a usable environment to build and run the interpreter on Android devices. Build inside Termux's home directory ($HOME) to avoid execution restrictions on external storage.

Quick Termux commands:

```sh
pkg update && pkg upgrade
pkg install clang make
cd ~/path/to/FC-Language-Written-In-C/src
make
./fc src/fcdemo/math.fc
```

Contributing and reporting issues

- Open issues for bugs or feature requests.
- Create small, focused pull requests for contributions.
- See src/Module guide.txt for instructions on adding native modules.

License

This project is published under the MIT License. See LICENSE for full details.

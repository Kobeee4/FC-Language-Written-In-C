# FCL (FuseCore Language) - written in C

A small but real programming language implementation written in C. This repo contains a working lexer, parser, interpreter, and a set of native modules (including AI and neural-network helpers). I maintain this project solo from my phone as a mobile developer.

Author: Kobeee4 (solo mobile developer)

Overview

FCL (FuseCore Language) is an experimental, lightweight language implemented in C. It is intended for learning, experimentation, and quick demos. The project already contains the main pieces of a working interpreter: a lexer, a parser, an AST, a runtime/interpreter, and many native modules exposed to scripts.

What is in the repo (high level)

- Working lexer: src/lexer.c, src/lexer.h
- Working parser: src/parser.c, src/parser.h
- Interpreter and runtime: src/interpreter.c, src/main.c
- Native modules and bindings: src/modules.c (includes ai and nn modules for matrix ops and neural helpers)
- Demos and scripts: src/fcdemo (math.ai demo), src/update (small example and a stub runtime I added for quick checks)
- Two Makefiles:
  - top-level Makefile I added to produce bin/fcl for an easy demo build
  - src/Makefile which builds the original "fc" interpreter (used by the original project structure)

Notable built-in modules

- ai: matrix, dot, add, transpose, train_linear, predict, randn, shape, apply
- nn: sigmoid, relu, tanh, softmax, and derivative helpers
- math, crypto, sec (security), web, file, random, os, and more

There are demo scripts showing AI and tiny neural forward passes in src/fcdemo/math.fc

Build options

You can build the project in two ways depending on what you want to run.

1) Use the top-level Makefile (easy demo build, produces bin/fcl)

   From the repository root:

   ```sh
   make
   ```

   The top-level Makefile compiles C sources (from src) into object files and links an executable at ./bin/fcl.

2) Use the original src/Makefile (the classic fc build)

   ```sh
   cd src
   make
   ```

   This produces the `fc` binary in the src directory (the old project layout). You can also build with:

   ```sh
   cc -O2 -std=gnu11 -o fc main.c lexer.c parser.c interpreter.c modules.c others.c -lm
   ```

Running programs

- Run the full interpreter (recommended for demos and real scripts):

  - If you used the top-level Makefile:

    ```sh
    ./bin/fcl path/to/script.fc
    ```

  - If you built with src/Makefile inside src/:

    ```sh
    ./fc path/to/script.fc
    ```

- Demos:

  - AI + NN demo (matrix ops and tiny neural forward pass):

    ```sh
    ./bin/fcl src/fcdemo/math.fc
    ```

  - Quick example I added (a small stub runtime that prints the file):

    ```sh
    ./bin/fcl src/update/example.fc
    ```

Notes about the stub in src/update

I added a minimal stub runtime in src/update/main.c to make a very low-friction smoke test and an example file at src/update/example.fc. The real interpreter in src/main.c is the full runtime and already supports the ai and nn modules; use that for real testing and demos.

Mobile/Termux notes

You mentioned you work from a phone. Good news: the project includes notes and has been developed with Termux in mind. See src/README.txt for detailed Termux instructions. Quick summary:

- Install Termux (F-Droid recommended) and packages: clang, make
- Build inside Termux's $HOME (do not build on /sdcard because of noexec)
- Example Termux commands:

  ```sh
  pkg update && pkg upgrade
  pkg install clang make
  cd ~/path/to/FC-Language-Written-In-C/src
  make
  ./fc src/fcdemo/math.fc
  ```

How to help or contribute

I maintain this solo from my phone, so small changes are best. If you want to contribute:

- Open small, focused pull requests
- Add examples or small modules (follow Module guide in src/Module guide.txt)
- File issues for bugs or feature requests

If you want, I can add CONTRIBUTING and ISSUE templates to make this easier for you and others.

License

This project is licensed under the MIT License. See LICENSE for details.

Thank you

If anything in this README still looks off, tell me what to fix and I will update it. I know you are tired; I can keep making small fixes, add more examples, or wire tests so you do not have to push changes yourself. Rest up and I will take care of the little stuff if you want.

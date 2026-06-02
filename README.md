# FCL (FuseCore Language) - written in C

A small, work in progress programming language implementation written in C. This repository contains the source for FCL, a personal project to explore language design, parsing, and runtime behavior. I build and maintain this project as a solo developer.

Author: Kobeee4 (solo mobile developer)

## Overview

FCL is an experimental language project. The goals are simple:
- learn and practice writing a language implementation in C
- explore parsing, AST design, and runtime evaluation
- keep the code readable and easy to experiment with

This is not production ready. It is a learning project and a codebase you can read, run, and extend.

## Current status

- Early prototype
- Core parts implemented in C
- Active work inside the `src` folder
- No stable release yet

## Features and goals

These are things I am working on or planning:
- a simple lexer and parser
- a compact AST representation
- an interpreter or a minimal code emitter
- clear C code so other developers can follow along

If you find missing parts or want to try improvements, contributions are welcome.

## Prerequisites

- A C compiler such as gcc or clang
- make
- A Unix like shell is recommended for the build steps

## Build

From the project root:

1. Open a terminal
2. Run:

   ```
   make
   ```

3. If the Makefile produces an executable, run it from the project root or the `bin` folder as directed by the build output.

If the build fails, check that you have a working C toolchain and that your PATH includes gcc or clang.

## Usage

Usage depends on the current build output. Typical steps:

- Build with `make`
- Run the produced executable with the appropriate arguments
- Look in the `src` folder for example code or the main entry point

If you want, I can add a concrete example and a sample FCL program once you tell me how you prefer to run it.

### Quick example (added)

This repository now includes a small example runtime stub and a sample program in `src/update` so you can build and try the project immediately.

1. Build the project:

   ```
   make
   ```

2. Run the example program:

   ```
   ./bin/fcl src/update/example.fc
   ```

   The runtime is a minimal stub that prints the contents of the file and simulates execution. This is meant to be a low-friction way to verify the build and iterate on language features.

## How to help

I am a solo maintainer. If you want to help:
- Open issues for bugs or feature ideas
- Send pull requests for small, focused changes
- Keep changes small and document behavior in comments or the README

I will review contributions when I can. Response time may vary because I am the only maintainer.

## License

This project is licensed under the MIT License. See `LICENSE` for details.

## Contact

- GitHub: https://github.com/Kobeee4

Thanks for checking out FCL. If you want more examples or a short tutorial, tell me how you want to run programs and I will add one.

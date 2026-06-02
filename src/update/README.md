# Minimal stub runtime for FCL

This folder contains a minimal C runtime that builds into `bin/fcl` when using the top-level Makefile.

To build and run the stub example:

  make
  ./bin/fcl src/update/example.fc

The stub runtime prints a header, reads the source file passed as the first argument, and prints its contents as a simulated execution output. The full interpreter and REPL are implemented in src/main.c and provide the complete runtime environment and native modules.

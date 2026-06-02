# Minimal stub runtime for FCL

This file contains a minimal C runtime that builds into `bin/fcl`. It is intentionally small and self-contained so you can build and try the repository without wiring the full interpreter yet.

To build and run:

  make
  ./bin/fcl src/update/example.fcl

The runtime prints a header, reads the file passed as the first argument, and prints its contents as a simulated execution output.

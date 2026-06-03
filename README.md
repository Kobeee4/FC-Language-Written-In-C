<p align="center">
  <img src="images/fc_logo.png" width="220">
</p>

<h1 align="center">FCL: FuseCore Language</h1>

<p align="center">
A complete, production-ready programming language built in C with lexer, parser, interpreter, and native module system
</p>

<p align="center">
  <img alt="License" src="https://img.shields.io/badge/license-MIT-blue.svg">
  <img alt="Language" src="https://img.shields.io/badge/language-C-00599C.svg">
  <img alt="Status" src="https://img.shields.io/badge/status-active-success.svg">
</p>

**Tags**: `programming-language` `interpreter` `compiler` `c-language` `lexer-parser` `neural-networks` `ai-modules` `open-source` `educational`

---

## Overview

FCL (FuseCore Language) is a modern, lightweight programming language implemented entirely in C. It provides a complete language implementation stack including a lexer, recursive descent parser, tree-walking interpreter, and an extensible native module system. FCL is designed for developers who want to understand how programming languages work, build domain-specific applications, or experiment with language design concepts.

## Key Features

- **Complete Language Stack**: Full lexer, parser, and AST-based interpreter
- **Interactive REPL**: Real-time code execution and experimentation
- **Native Module System**: Built-in support for AI, neural networks, cryptography, web operations, and more
- **Cross-Platform**: Works on Linux, macOS, and Android (via Termux)
- **Educational Focus**: Clean, readable source code designed for learning
- **Extensible Architecture**: Easy to add custom native modules

## Repository Structure

- **src/**: Core interpreter implementation
  - `lexer.c/h`: Tokenization and lexical analysis
  - `parser.c/h`: Syntax analysis and AST construction
  - `interpreter.c`: Execution engine
  - `modules.c`: Native module implementations (AI, NN, math, crypto, web, file, OS, random, security)
  - `main.c`: REPL and CLI interface
  - `fcdemo/`: Complete working examples
- **docs/**: User and developer documentation
- **bin/**: Compiled executables
- **LICENSE**: MIT License

## Quick Start

### Build

From the repository root:

```sh
make
```

Or build from source directory:

```sh
cd src
make
```

### Run Scripts

```sh
./bin/fcl path/to/script.fc
```

### Examples

AI and neural network demo with matrix operations:

```sh
./bin/fcl src/fcdemo/math.fc
```

Quick example script:

```sh
./bin/fcl src/update/example.fc
```

## Platform Support

### Linux and macOS

Standard build with gcc or clang:

```sh
make
./bin/fcl your_script.fc
```

### Android (Termux)

Build and run on Android devices using Termux:

```sh
pkg update && pkg upgrade
pkg install clang make
cd ~/path/to/FC-Language-Written-In-C/src
make
./fc src/fcdemo/math.fc
```

## Module System

FCL includes native modules for:

- **AI**: Machine learning and AI operations
- **Neural Networks**: Neural forward propagation and matrix operations
- **Cryptography**: Encryption and secure operations
- **Web**: HTTP requests and web integration
- **File I/O**: File and directory operations
- **OS**: Operating system interfaces
- **Math**: Advanced mathematical functions
- **Random**: Pseudorandom number generation
- **Security**: Security-focused utilities

For details on creating custom modules, see `src/Module guide.txt`.

## Contributing

We welcome contributions from the community:

- Report bugs or request features via GitHub Issues
- Submit focused, well-documented pull requests
- Follow the coding style in the existing codebase
- See `src/Module guide.txt` for native module development guidelines

## License

This project is released under the MIT License. See the LICENSE file for complete terms and conditions.

---

**Built with passion for language design and open source education.**

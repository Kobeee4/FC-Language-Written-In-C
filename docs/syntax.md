# Syntax Reference — FCL (FuseCore Language)

This is a short, practical syntax guide for FCL so you can read and write small programs quickly.

File extension: .fc

Comments
- Full-line comments begin with a plus and a space: `+ this is a comment`

Indentation and blocks
- Blocks are defined by indentation (similar to Python). The lexer emits INDENT and DEDENT tokens.
- Use consistent indentation (4 spaces conventional). Mismatched indentation is a syntax error.

Primitives
- Number: `123`, `3.14`
- String: double-quoted: `"hello"`
- Boolean: `true`, `false`
- Null: `null`

Variables and declarations
- Let binding: `let x = 5`
- Assignment: `x = 10`

Control flow
- If/elif/else
  func example(x)
      if x > 0
          out "positive"
      elif x == 0
          out "zero"
      else
          out "negative"

- Loops
  - `loop` with `times` for simple counted loops
  - `while`, `for`, `break`, `continue`

Functions and lambdas
- Define: `func name(param1, param2)` followed by an indented block
- Return with `return`
- Lambdas and higher-order functions available via `fn.*` helpers

Data structures
- Lists: `[1, 2, 3]`
- Dicts: `{ "key": "value" }`

I/O and modules
- Print: `out "text"` or use `print(...)` depending on demo style
- Import native modules are available as global names (e.g. `ai`, `nn`, `math`, `file`, `web`)
  - Example: `let m = ai.matrix(2, 2)`

Example program
```
+ simple example
let x = 10
func inc(v)
    return v + 1
out inc(x)
```

Notes
- See `src/README.txt` for a longer language overview and module documentation.
- The interpreter and REPL are in `src/main.c`; run scripts with `./bin/fcl script.fc` or `./src/fc script.fc` depending on your build method.

# Syntax Reference — FCL (FuseCore Language)

File extension: .fc

Comments
- Full-line comments begin with a plus and a space: `+ this is a comment`

Indentation and blocks
- Blocks are defined by indentation. The lexer emits INDENT and DEDENT tokens.
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
  Example:
  ```
  func example(x)
      if x > 0
          out "positive"
      elif x == 0
          out "zero"
      else
          out "negative"
  ```

- Loops: `loop`, `times`, `while`, `for`, `break`, `continue`

Functions and lambdas
- Define: `func name(param1, param2)` followed by an indented block
- Return with `return`
- Higher-order functions available via `fn.*` helpers

Data structures
- Lists: `[1, 2, 3]`
- Dicts: `{ "key": "value" }`

I/O and modules
- Print: `out "text"` or use module functions as demonstrated in demos
- Native modules are available by name (for example `ai`, `nn`, `math`, `file`, `web`)

Example program

```
+ simple example
let x = 10
func inc(v)
    return v + 1
out inc(x)
```

Reference
- For a longer language overview and module documentation see `src/README.txt`.

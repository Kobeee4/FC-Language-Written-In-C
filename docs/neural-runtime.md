# Neural runtime and AI modules — FCL

FCL includes helpers for matrix arithmetic and neural-network utilities. These functions are implemented as native modules in C and operate on the interpreter's list-of-list structures.

### Note:
You can write a custom module/package with proper AI stuff. 

Provided modules

- ai: matrix and numeric helpers
  - ai.matrix(rows, cols)       -> creates a zero-filled matrix (list of lists)
  - ai.randn(rows, cols)        -> random matrix with values in [-1, 1]
  - ai.dot(m1, m2)              -> matrix multiplication
  - ai.add(m1, m2) / ai.sub     -> element-wise add/subtract
  - ai.scale(m, k)              -> multiply every element by k
  - ai.transpose(m)             -> transpose matrix
  - ai.apply(func, m)           -> apply function element-wise
  - ai.shape(m)                 -> return [rows, cols]
  - ai.train_linear(x_list, y_list) -> least-squares fit (returns {"m":, "b":})
  - ai.predict(model, input)    -> predict using the linear model

- nn: activation and neural helpers
  - nn.sigmoid(x), nn.relu(x), nn.tanh(x), nn.leaky_relu(x)
  - nn.softmax(list)            -> normalized probabilities
  - nn.dsigmoid(x), nn.drelu(x), nn.dtanh(x) -> derivative helpers

Usage example

```
let inp = [[0.5]]
let w1 = [[0.1, 0.2]]
let hidden = ai.apply(nn.sigmoid, ai.dot(inp, w1))
let w2 = [[0.7], [0.3]]
let output = ai.apply(nn.sigmoid, ai.dot(hidden, w2))
out output
```

Limitations

- These helpers are simple and not optimized for large-scale numerical workloads.
- For performance-sensitive tasks, linking to an optimized native backend (BLAS) or providing an FFI is recommended.

Possible Improvements

- Add matrix operation optimizations or an optional native backend
- Introduce batched operations and basic training loops
- Add unit tests and numeric stability checks for core operations

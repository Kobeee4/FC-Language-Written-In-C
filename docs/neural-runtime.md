# Neural runtime and AI modules — FCL

FCL ships with built-in helpers for matrix arithmetic and lightweight neural-network utilities. This page explains the available functions and how to use them.

Two related modules are provided in src/modules.c: `ai` and `nn`.

ai module (matrix and helpers)
- ai.matrix(rows, cols)       -> creates a zero-filled matrix (list of lists)
- ai.randn(rows, cols)        -> random matrix in [-1, 1]
- ai.dot(m1, m2)             -> matrix multiplication
- ai.add(m1, m2) / ai.sub    -> element-wise add/subtract
- ai.scale(m, k)             -> scale every element by k
- ai.transpose(m)            -> transpose matrix
- ai.apply(func, m)          -> apply function element-wise
- ai.shape(m)                -> return [rows, cols]
- ai.train_linear(x_list, y_list) -> simple least-squares fit returning {"m":, "b":}
- ai.predict(model, input)   -> predict with linear model (handles scalars or lists)

nn module (activation and helpers)
- nn.sigmoid(x)  nn.relu(x)  nn.tanh(x)  nn.leaky_relu(x)
- nn.softmax(list) -> normalized probabilities
- nn.dsigmoid(x), nn.drelu(x), nn.dtanh(x) -> derivative helpers

Usage example (tiny forward pass)
```
let inp = [[0.5]]
let w1 = [[0.1, 0.2]]
let hidden = ai.apply(nn.sigmoid, ai.dot(inp, w1))
let w2 = [[0.7], [0.3]]
let output = ai.apply(nn.sigmoid, ai.dot(hidden, w2))
out output
```

Notes and limitations
- These helpers are simple and implemented in C as native functions. They are not optimized BLAS-level implementations and operate on the interpreter's list-of-list structures.
- For small models and demos (toy linear fits, tiny forward passes) they are fine. For real training on large data, a native optimized backend would be needed.
- The `ai.train_linear` implements a least-squares linear fit (good for demonstrations). There is no SGD or batched training yet.

Possible improvements
- Add matrix memory layout optimizations or an FFI to link with a native BLAS / vector library
- Implement batch operations and a small autodiff engine for training
- Provide more examples and unit tests for numeric stability

If you want I can add a dedicated demo that shows training a small dataset end-to-end and saves results to src/fcdemo/ai_train_demo.fc.
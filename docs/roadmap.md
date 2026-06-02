# Roadmap — FCL (concise)

A short, practical roadmap with actionable items.

Short term (low effort, high value)

- Add automated smoke tests that build and run `src/fcdemo/math.fc` and `src/update/example.fc`.
- Update the stub runtime in src/update to parse and execute basic print statements.
- Add CONTRIBUTING.md and issue templates.
- Improve README examples with explicit build-and-run steps for mobile/Termux users.

Medium term (helpful features)

- Add GitHub Actions CI to run `make` on pushes and pull requests.
- Add more demos: ai_train_demo.fc, nn_tiny_demo.fc.
- Add parser and lexer unit tests to catch regressions.
- Expand documentation for the interpreter Value model and module API.

Long term (bigger projects)

- Optimize matrix operations or add an optional native backend (BLAS).
- Implement a bytecode compiler and VM for faster execution.
- Implement a basic autodiff engine and training loop for the nn helpers.

Contributor-friendly tasks

- Implement small native modules following the Module guide in src/Module guide.txt.
- Port demos to use real data and include expected outputs for CI.
- Keep changes small and self-contained to simplify mobile review.

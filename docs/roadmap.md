# Roadmap — FCL

This is a focused roadmap with small, actionable items you or contributors can pick up. I kept it lightweight because you maintain this solo from a phone.

Short term (low effort, high value)
- Add unit smoke tests that build and run `src/fcdemo/math.fc` and `src/update/example.fc` automatically
- Fix the update stub so it executes `print("...")` instead of echoing the file
- Add CONTRIBUTING.md and a simple issue template
- Improve README examples with concrete build-and-run steps for mobile/Termux users

Medium term (helpful features)
- Add GitHub Actions CI to run `make` on push and PRs
- Add more demos: ai_train_demo.fc, nn_tiny_demo.fc
- Add small parser tests to catch regressions in lexer/parser
- Add basic docs for the interpreter Value model and module API

Longer term (bigger projects)
- Optimize matrix ops or add an optional native backend (BLAS)
- Add a small bytecode compiler for faster execution
- Implement a tiny autodiff engine and training loop for the nn helpers

Contributor-friendly tasks
- Add small modules (see `src/Module guide.txt`) and document them
- Port demos to use more real data and include expected outputs for CI tests

Notes about maintenance
- Keep changes small and self-contained so you can review them easily on mobile
- Tag releases with short notes to make demo builds reproducible

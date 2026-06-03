# Contributing

Thanks for helping improve FC Language (FCL)! 
Quick ways to contribute
- Join the Discord server: https://discord.gg/pP9Ur2FnRA — share patches, ask for reviews, or request help.  
- Fork the repo, make changes, push to your fork, and open a Pull Request against the main branch.

What we accept
- Source code only: submit source files, build scripts, and small test data. Do NOT include compiled binaries, installers, or large generated artifacts in PRs. If you must include a generated artifact for a demo, keep it small and add the steps to reproduce it in the PR description.
- Packages: you may publish reusable packages/modules. Preferred approaches:
  - Add the package under src/packages/<package-name> with a short metadata file (README and a small manifest describing name, version, entry point, and dependencies).
  - Or publish as a separate public repo (recommended for larger packages) and link to it from src/packages/README.md.
  - Always include build/install instructions and a short usage example (demo or tests).
- C dialects: any standard C (C89/C99/C11/C17, etc.) is allowed as long as the code builds and documents required compiler flags. If using non-standard extensions, call them out and provide alternative or fallbacks where possible.

Short PR checklist
1. Fork → git checkout -b feature/short-desc → implement → commit → push → open PR.  
2. PR description should include: summary, testing performed, and any related issue.  
3. Keep changes small and focused. Prefer multiple small PRs over one large PR.  
4. Include build instructions and explain any new dependencies.  
5. Add or update a demo in src/fcdemo or tests for functional changes where practical.

Coding & packaging guidance
- Keep code readable, commented, and modular. Avoid large monolithic files when possible.  
- Document public APIs (headers) and module entry points.  
- For packages/modules, include:
  - README.md (purpose, usage example)
  - manifest (name, version, entry file, dependencies)
  - build instructions (compiler flags, required tools)
- Respect the repo MIT license — contributions are licensed under MIT.

Security & private issues
- Do not include secrets in PRs.  
- To report security issues privately, contact maintainers via Discord before public disclosure.

Need help?
- Post code snippets or links on Discord and someone will review or help apply small patches.

Wanna Report A Bug? 
Join The Discord Server

Thanks — looking forward to your contributions!
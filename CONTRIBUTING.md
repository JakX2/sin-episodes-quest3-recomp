# Contributing to SiN Episodes Quest 3 compatibility

Thanks for helping turn research into a playable, well-tested project. **This
repository is a portable research prototype, not yet a game port.** Contributions
that make the code buildable, testable, and usable by another developer are
particularly valuable.

## Get started

1. Read [README.md](README.md), [docs/STATUS.md](docs/STATUS.md), and
   [docs/ROADMAP.md](docs/ROADMAP.md).
2. Build and run the tests on Linux, macOS, or a suitable Windows toolchain:

   ```sh
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --parallel
   ctest --test-dir build --output-on-failure
   ```

3. Pick a small, testable issue or propose a scoped design in a GitHub issue.
4. Fork the repository, create a feature branch, and submit a PR explaining the
   behavior, limitations, and test commands.

## What a good pull request includes

- A concrete problem and a minimal implementation, rather than a huge rewrite.
- Unit or integration tests. Report actual pass/fail results, not estimated
  assertion counts.
- For reverse-engineering claims, the source (binary hash/build, function offset,
  observable behavior, or reproducible experiment) and a clear confidence level.
- When a test uses synthetic values, label them as synthetic. Keep speculative
  engine behavior distinct from verified binary behavior.
- A note in `docs/STATUS.md` or `docs/HANDOFF.md` if a milestone, compatibility
  assumption, or integration contract changes.

## Legal and safety boundaries

**Do not submit proprietary game DLLs, assets, maps, sounds, models, weapon
scripts, game archives, leaked engine SDK files, or disassembly dumps copied
from the original product.** Keep extracted game content in your own local
workspace. This project contains independent compatibility code and research
references; [docs/IP_AND_ASSETS.md](docs/IP_AND_ASSETS.md) explains the boundary.

All code contributions are submitted under the repository's MIT license unless
clearly identified as third-party code with a compatible license and provenance.
Please don't submit others' copyrighted code without permission.

## Project maintainership

Anyone can fork this project. The original maintainer need not be active for
forks to continue. If you're taking on a larger feature or proposing a
maintainer transition, open an issue documenting your intended scope and
how you will test it. The repository history, CI and the handoff checklist are
intended to keep the work reproducible.

# Repository Guidelines

## Project Structure & Module Organization
`src/` contains the production code, split by subsystem (`common`, `daemon`, `library`, `metrics`, `management`, `tools`, `slurm_plugin`, `pbs_hook`, etc.). Architecture-specific code usually lives under `archs/`, and shared helpers are in `src/common`. Tests live in `tests/` plus some module-local test folders such as `src/data_center_monitor/plugins/tests/`. `etc/` holds install templates, systemd units, examples, and RPM assets. `doc/` and `m4/` support documentation and autotools.

## Build, Test, and Development Commands
Run `./configure [options]` first to generate `Makefile`s and config headers. Common flags include `--prefix=...`, `--with-slurm=...`, `--with-cuda=...`, and `--disable-mpi`. Use `make` for an incremental build, `make full` for `clean && all`, `make install` to install into the configured prefix, `make test` to build and run the top-level tests, and `make clean` to remove build outputs.

## Coding Style & Naming Conventions
C sources follow the repo's `.clang-format` style: Microsoft-based formatting, 4-space indentation, right-aligned pointers, LF line endings, and aligned assignments/macros. Keep existing `/* clang-format off */` regions intact when they protect tables or hand-aligned layouts. Use lowercase snake_case for files, functions, variables, and tests. Prefer subsystem prefixes such as `eard_`, `mgt_`, `powercap_`, and `thermal_`.

## Testing Guidelines
Top-level tests are plain C executables named `test_*.c` or `*_tests.c`. Run `make test` after building; it compiles the `tests/Makefile.in` targets and executes them sequentially. Some module tests require optional dependencies such as `cmocka`, so keep test code deterministic and mock external calls explicitly.

## Commit & Pull Request Guidelines
Git history favors short imperative subjects, for example `Fix unused result warnings` or `Update command help and man pages`, sometimes with PR refs like `(#495)`. Keep commits focused and mention any build or configure impact in the body. PRs should include a concise summary, the commands you ran, relevant `./configure` options, and screenshots only for documentation or UI changes.

## Generated Files
Edit autotools inputs (`configure.ac`, `*.am`, `*.in`) rather than generated outputs when possible, and rerun the relevant build steps after changing build logic or templates.

# Repository Guidelines

## Project Structure & Module Organization

EAR is a C/Autotools project.
- `src`: production code.
- `src/common/`: for shared utilities
- `src/library/`: EARL runtime code
- `src/daemon/`: EARD services/APIs.
- `src/metrics/` and `src/management/`: for hardware back ends (the former for retrieving data and the latter for making changes).
- `src/report/`: data report plugins
- `src/commands/` and `src/tools/`: CLIs which compile to binaries directly.
- `src/slurm_plugin/` has the code which interacts with the SLURM scheduler and also the implementation of the `erun` command, which simulates SLURM's `srun` command to be used on non-SLURM systems.
- `src/pbs_hook/`: stuff related with this other scheduler.
- `src/data_center_monitor/`: a component which uses the plugin\_manager implemented under `src/common/` to make the edcmon, which is the EAR Data Center Monitor service, implementing a set of plugins to monitor, optimize and report data.
- `src/tests/`: Top-level and/or e2e tests.
- `src/*/test(s)?/`: Component tests, normally unit tests.
- `etc/`: Configuration templates, services, SQL, RPM specs, modules, and images.
- `m4/` and `build-aux/`: Autotools support.

## Build, Test, and Development Commands

- It is not guaranteed that calling `make -C any/repo/directory` works, because repo root `Makefile.exports` needs to be called. It is normally better to try to compile the whole project from the root directory:
	- `autoreconf -i`: regenerate Autotools files after editing `configure.ac` or `m4/*.m4`.
	- `./configure [options]`: generate `Makefile`; CI examples include `./configure --with-eaas` and `./configure --with-eaas --enable-kubernetes-support --without-slurm`.
        - You can use `CONFIG_SITE=<config>.site ./configure` if you locate the `<config>.site` config site file.
    - Due to a a known issue with searching for MPI flags, it is recommended to configure ALWAYS, at least,  with `--disable-mpi --disable-mpi-loader` among other flags unless explicitely requested.
	- `make`: build the configured tree.
	- `make full`: clean and rebuild. Use this always unless a single file is modified.
        - If there is a trouble related with `am--refresh` error, run `autoreconf -i` and `configure` directives again.
	- `make post`: build post targets, including optional component tests.
	- `make test`: run the top-level test harness in `tests/`.
	- `make clean` and `make test-clean`: remove build and test outputs.
	- `make install` and `make etc.install`: install binaries and generated configuration using the configured prefix.
        - `make etc.install` is needed just the first time EAR is installed unless new files in `etc/` need to be installed.

Components/directories which currently use Automake (i.e., there is a `Makefile.am` file inside) provide standard Make targets, e.g., `all`, `install`, `clean`, `check`, where `check` is equivalent to `post` in other directories' Makefiles. `post` target in standalone `Makefile`s might call `check` target in subdirectories built with Automake. Some non-standard targets are added in `Makefile.am` to prevent `make` invokation not crashing.

When running `make test`, `test_thermal_actions` might fail. If that happens, ignore it.

After editing any file, run `clang-tidy-diff.py` (may have a version suffixed, e.g., `clang-tidy-diff-<version>.py`) to run `clang-tidy` on the lines that have been modified in your working directory or in a specific diff. Example invocations:

```sh
# Run clang-tidy on all changes in the working directory
$ git diff -U0 --no-color HEAD^ | clang-tidy-diff.py -p1

# Run with specific checks and apply fixes
$ git diff -U0 --no-color HEAD^ | clang-tidy-diff.py -p1 -fix \
  -checks=-*,readability-*

# Run on staged changes
$ git diff -U0 --no-color --cached | clang-tidy-diff.py -p1

# Run on changes between two commits
$ git diff -U0 --no-color HEAD~2 HEAD | clang-tidy-diff.py -p1

# Run on a patch file
$ clang-tidy-diff.py -p1 < changes.patch
```

## Coding Style & Naming Conventions
Use `.clang-format` (ignored files in `.clang-format-ignore`): Microsoft-based style, 4-space indentation, LF line endings, right-aligned pointers (`char *name`), aligned macros/assignments, and separated definition blocks. Format C and header changes before review:

```sh
find src/ -name "*.[ch]" -exec clang-format-<version> -i {} +
```

`clang-format` command is normally suffixed with `-<version>`. Find it before calling it. Prioritize the highest version.
Keep existing `/* clang-format off */` regions intact when they protect tables or hand-aligned layouts.

Keep C files and headers in lower snake case, usually pairing `module.c` with `module.h`. Prefer subsystem prefixes such as `eard_`, `mgt_`, `powercap_`, and `thermal_`. Use uppercase for macros and constants. Do not leave active debug switches such as `SHOW_DEBUGS` or `DEBUG_ENABLED 1`; CI rejects them.

### Copyright notice

All source and header files, i.e., `*.[ch]`, must have the following Copyright notice on the top if they belong to BSC public version. Add it when creating new files, or modify the year if it doesn't match the current year.

```c
/***************************************************************************
 * Copyright (c) 2026 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/
```

## Testing Guidelines
Top-level tests are plain C executables named `test_*.c` or `*_tests.c`. Run `make test` after building; it compiles the `tests/Makefile.in` targets and executes them sequentially. Some module tests require optional dependencies such as `cmocka`, so keep test code deterministic and mock external calls explicitly.

## Commit & Pull Request Guidelines
Git history favors short imperative subjects, for example `Fix unused result warnings` or `Update command help and man pages`, sometimes with PR refs like `(#495)`. Keep commits focused and mention any build or configure impact in the body. PRs should include a concise summary, the commands you ran, relevant `./configure` options, and screenshots only for documentation or UI changes.

## Generated Files
Edit autotools inputs (`configure.ac`, `*.am`, `*.in`) rather than generated outputs when possible, and rerun the relevant build steps after changing build logic or templates.

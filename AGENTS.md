# AGENTS.md

This file provides guidance to AI coding assistants when working with
code in this repository.

## Build and Test

The build system is Meson (the legacy Makefile still works but is
deprecated).

```sh
# Configure and build
meson setup build
meson compile -C build

# Run all tests
meson test -C build

# Run a specific test suite (libfdt, dtc, fdtget, fdtput, fdtdump, fdtoverlay, pylibfdt, utilfdt, dtbs_equal)
meson test -C build dtc
meson test -C build libfdt

# Legacy make (deprecated, still functional)
make
make check          # all tests
make checkm         # tests under valgrind
```

Optional build dependencies: libyaml (>= 0.2.3) for YAML output,
valgrind for memory checking, swig + python3-dev for pylibfdt.

## Architecture

The repo contains three main components:

### dtc (Device Tree Compiler)

Compiles device tree source (.dts) to binary (.dtb) and vice versa. The pipeline is: parse source → live tree → flatten to blob (or reverse).

- **Parsing**: `dtc-lexer.l` (flex) + `dtc-parser.y` (bison) produce a live tree from .dts source. `flattree.c` reads .dtb blobs. `fstree.c` reads /proc/device-tree style filesystem trees. `yamltree.c` writes YAML output.

- **Live tree** (`livetree.c`, `dtc.h`): In-memory representation as `struct node` / `struct property` trees with labels, phandles, and source position tracking. The `struct data` type carries property values with type markers and cross-reference markers.
- **Checks** (`checks.c`): ~50 semantic checks registered via `WARNING()`, `ERROR()`, and `CHECK()` macros into a `check_table[]`. Each check declares prerequisite checks, forming a DAG. Checks validate DT conventions (node naming, property types, interrupt structures, etc.). Use `-W`/`-E` flags to promote/demote.
- **Output**: `flattree.c` writes .dtb blobs and assembler output. `treesource.c` writes .dts source.

### libfdt (Flat Device Tree library)

C library for reading/writing .dtb blobs in-place, dual-licensed
GPL-2.0-or-later OR BSD-2-Clause. Used in bootloaders, kernels, and
hypervisors where the full compiler isn't available.

- `fdt_ro.c` — read-only access (property lookup, node traversal)
- `fdt_rw.c` — read-write modification of existing blobs
- `fdt_sw.c` — sequential-write creation of new blobs
- `fdt_wip.c` — "write in place" operations (in-place modification)
- `fdt_overlay.c` — device tree overlay application
- `fdt_check.c` — blob validation (`fdt_check_full`)
- `fdt_addresses.c` — address/size cell helpers
- `version.lds` — exported symbol list; new public functions must be added here

libfdt is designed to be embeddable: `Makefile.libfdt` can be included
by external build systems. The `FDT_ASSUME_MASK` controls safety
vs. performance tradeoffs (see `libfdt_internal.h`).

### pylibfdt

SWIG-generated Python bindings for libfdt
(`pylibfdt/libfdt.i`). Functions not supportable by SWIG should be
behind `#ifndef SWIG` in `libfdt.h`.

## Tests

Tests live in `tests/`. The test runner is `tests/run_tests.sh` which
defines test groups: `libfdt_tests`, `dtc_tests`, `fdtget_tests`,
`fdtput_tests`, `fdtoverlay_tests`, `pylibfdt_tests`, etc.

Individual C test programs link against libfdt and use helpers from
`tests/testutils.c`. Binary test trees are built from assembler macros
in `tests/trees.S` via `tests/dumptrees.c` — if you modify
`tests/test_tree1.dts`, you must also update `tests/trees.S`.

## AI Contribution Policy

See the "AI Coding Assistants" section in CONTRIBUTING.md. Key rules:

- **Do not** add `Signed-off-by` tags — only humans can certify the DCO
- Use `Assisted-by: AGENT_NAME:MODEL_VERSION [TOOL1] [TOOL2]` for attribution in commit messages
- The human submitter is responsible for reviewing all AI-generated code and ensuring license compliance

## Tagging a New Release

Releases use the `vMAJOR.MINOR.PATCH` tag format (e.g., `v1.7.2`).

An AI agent can prepare the release; the human maintainer reviews,
adds `Signed-off-by`, and signs the tag.

1. **Update `VERSION.txt`** — change the single line to the new version
   number (e.g., `1.7.2`). `meson.build` reads its version from this
   file. No other version files need editing.

2. **Draft the version bump commit** — create the commit *without*
   `Signed-off-by` (agents must not add S-o-b per the AI contribution
   policy). Use the message format:
   ```
   Bump version to vX.Y.Z

   Prepare for a new release.
   ```

3. **Draft the tag message** — write it to a temporary file (e.g.,
   `tag-message.txt`) for the maintainer to review. The format is:
   ```
   DTC X.Y.Z

   Changes since vPREV include:
    * Component
      - Change description
      - ...
   ```
   Group changes by component (dtc, libfdt, pylibfdt, fdtget,
   fdtoverlay, Build, General, etc.) with a bullet per notable change.
   Generate the changelog from `git log vPREV..HEAD`.

4. **Human review** — the maintainer reviews the commit and tag
   message, then runs the finalize script which amends the commit to
   add `Signed-off-by` and creates the signed annotated tag:
   ```sh
   scripts/finalize-release tag-message.txt
   ```

Agents must **never** run `scripts/finalize-release` or
`scripts/kup-dtc`. These perform signing and upload operations that
only a human maintainer may execute.

## Coding Conventions

- License: GPL-2.0-or-later for dtc tools; (GPL-2.0-or-later OR BSD-2-Clause) for libfdt
- SPDX identifiers on every file
- C style follows kernel conventions: tabs for indentation, `lower_case` names
- Compiler warnings are errors (`-Werror`)
- libfdt functions return negative `FDT_ERR_*` codes on failure (never errno)

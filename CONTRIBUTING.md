# Contributing to dtc or libfdt

There are two ways to submit changes for dtc or libfdt:

* Post patches directly to the the
  [devicetree-compiler](mailto:devicetree-compiler@vger.kernel.org)
  mailing list.
* Submit pull requests via
  [Github](https://github.com/dgibson/dtc/pulls)

## Adding a new function to libfdt.h

The shared library uses `libfdt/version.lds` to list the exported
functions, so add your new function there. Check that your function
works with pylibfdt. If it cannot be supported, put the declaration in
`libfdt.h` behind `#ifndef SWIG` so that swig ignores it.

## Tests

Test files are kept in the `tests/` directory. Use `make check` to build and run
all tests.

If you want to adjust a test file, be aware that `tree_tree1.dts` is compiled
and checked against a binary tree from assembler macros in `trees.S`. So
if you change that file you must change `tree.S` also.

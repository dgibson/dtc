# Contributing to dtc or libfdt

There are two ways to submit changes for dtc or libfdt:

* Post patches directly to the
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

## Developer's Certificate of Origin

Like many other projects, dtc and libfdt have adopted the "Developer's
Certificate of Origin" (Signed-off-by) process created by the Linux
kernel community to improve tracking of who did what.  Here's how it
works (this is a very slight modification of the description from
`Documentation/process/submitting-patches.rst` in the kernel tree):

The sign-off is a simple line at the end of the explanation for the
patch, which certifies that you wrote it or otherwise have the right
to pass it on as an open-source patch.  The rules are pretty simple:
if you can certify the below:

    Developer's Certificate of Origin 1.1

    By making a contribution to this project, I certify that:

        (a) The contribution was created in whole or in part by me and I
            have the right to submit it under the open source license
            indicated in the file; or

        (b) The contribution is based upon previous work that, to the best
            of my knowledge, is covered under an appropriate open source
            license and I have the right under that license to submit that
            work with modifications, whether created in whole or in part
            by me, under the same open source license (unless I am
            permitted to submit under a different license), as indicated
            in the file; or

        (c) The contribution was provided directly to me by some other
            person who certified (a), (b) or (c) and I have not modified
            it.

        (d) I understand and agree that this project and the contribution
            are public and that a record of the contribution (including all
            personal information I submit with it, including my sign-off) is
            maintained indefinitely and may be redistributed consistent with
            this project or the open source license(s) involved.

then you just add a line saying::

	Signed-off-by: Random J Developer <random@developer.example.org>

using your real name (sorry, no pseudonyms or anonymous
contributions.)  This will be done for you automatically if you use
`git commit -s`.  Reverts should also include "Signed-off-by". `git
revert -s` does that for you.

Any further SoBs (Signed-off-by:'s) following the author's SoB are
from people handling and transporting the patch, but were not involved
in its development. SoB chains should reflect the **real** route a
patch took as it was propagated to the maintainers, with the first SoB
entry signalling primary authorship of a single author.

## AI Coding Assistants

AI tools helping with dtc/libfdt development must follow the standard
contribution process described in this document.

### Licensing and Legal Requirements

All contributions must comply with the project's licensing requirements:

* All code must be compatible with GPL-2.0-or-later
  * All libfdt code must also be compatible with BSD-2-Clause
* Use appropriate SPDX license identifiers

### Signed-off-by and Developer Certificate of Origin

AI agents MUST NOT add Signed-off-by tags. Only humans can legally
certify the Developer Certificate of Origin (DCO). The human submitter
is responsible for:

* Reviewing all AI-generated code
* Ensuring compliance with licensing requirements
* Adding their own Signed-off-by tag to certify the DCO
* Taking full responsibility for the contribution

### Attribution

When AI tools contribute to development, proper attribution helps track
the evolving role of AI in the development process.  Contributions
should include an `Assisted-by` tag in the following format:

```
Assisted-by: AGENT_NAME:MODEL_VERSION [TOOL1] [TOOL2]
```

Where:

* `AGENT_NAME` is the name of the AI tool or framework
* `MODEL_VERSION` is the specific model version used
* `[TOOL1] [TOOL2]` are optional specialized analysis tools used
  (e.g., coccinelle, sparse, smatch, clang-tidy)

Basic development tools (git, gcc, make, editors) should not be listed.

Example:
```
Assisted-by: Claude:claude-3-opus coccinelle sparse
```

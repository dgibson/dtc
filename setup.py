#!/usr/bin/env python3
# SPDX-License-Identifier: (GPL-2.0-or-later OR BSD-2-Clause)

"""
setup.py file for SWIG libfdt
Copyright (C) 2017 Google, Inc.
Written by Simon Glass <sjg@chromium.org>
"""

import os
import sys

from setuptools import setup, Extension
from setuptools.command.build_py import build_py as _build_py


srcdir = os.path.dirname(__file__)

with open(os.path.join(srcdir, "README.md"), "r", encoding='utf-8') as fh:
    long_description = fh.read()

with open(os.path.join(srcdir, "VERSION.txt"), "r", encoding='utf-8') as fh:
    version = fh.readline().strip()

def get_top_builddir():
    """Figure out the top-level directory containing the source code

    Returns:
        str: Directory to build in
    """
    if '--top-builddir' in sys.argv:
        index = sys.argv.index('--top-builddir')
        sys.argv.pop(index)
        return sys.argv.pop(index)
    return srcdir

top_builddir = get_top_builddir()

libfdt_module = Extension(
    '_libfdt',
    sources=[os.path.join(srcdir, 'pylibfdt/libfdt.i')],
    define_macros=[('PY_SSIZE_T_CLEAN', None)],
    include_dirs=[os.path.join(srcdir, 'libfdt')],
    libraries=['fdt'],
    library_dirs=[os.path.join(top_builddir, 'libfdt')],
    swig_opts=['-I' + os.path.join(srcdir, 'libfdt')],
)


class BuildPy(_build_py):
    """Small class to run the build_ext command"""
    def run(self):
        self.run_command("build_ext")
        return super().run()


setup(
    name='libfdt',
    version=version,
    cmdclass = {'build_py' : BuildPy},
    author='Simon Glass',
    author_email='sjg@chromium.org',
    description='Python binding for libfdt',
    ext_modules=[libfdt_module],
    package_dir={'': os.path.join(srcdir, 'pylibfdt')},
    py_modules=['libfdt'],
    python_requires=">=3.8",

    long_description=long_description,
    long_description_content_type="text/plain",
    url="https://git.kernel.org/pub/scm/utils/dtc/dtc.git",
    license="BSD",
    license_files=["GPL", "BSD-2-Clause"],

    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: BSD License",
        "License :: OSI Approved :: GNU General Public License v2 or later (GPLv2+)",
        "Operating System :: OS Independent",
    ],
)

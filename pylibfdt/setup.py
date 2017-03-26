#!/usr/bin/env python

"""
setup.py file for SWIG libfdt

Files to be built into the extension are provided in SOURCES
C flags to use are provided in CPPFLAGS
"""

from distutils.core import setup, Extension
import os
import sys

progname = sys.argv[0]
files = os.environ['SOURCES'].split()
cflags = os.environ['CPPFLAGS'].split()

libfdt_module = Extension(
    '_libfdt',
    sources = files,
    extra_compile_args = cflags
)

setup (name = 'libfdt',
       version = '0.1',
       author      = "Simon Glass <sjg@chromium.org>",
       description = """Python binding for libfdt""",
       ext_modules = [libfdt_module],
       py_modules = ["libfdt"],
       )

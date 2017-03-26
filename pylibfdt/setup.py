#!/usr/bin/env python

"""
setup.py file for SWIG libfdt

Files to be built into the extension are provided in SOURCES
C flags to use are provided in CPPFLAGS
Object file directory is provided in OBJDIR
"""

from distutils.core import setup, Extension
import os
import sys

progname = sys.argv[0]
files = os.environ['SOURCES'].split()
cflags = os.environ['CPPFLAGS'].split()
objdir = os.environ['OBJDIR']
version = os.environ['VERSION']

libfdt_module = Extension(
    '_libfdt',
    sources = files,
    extra_compile_args = cflags
)

setup (name = 'libfdt',
       version = version,
       author      = "Simon Glass <sjg@chromium.org>",
       description = """Python binding for libfdt""",
       ext_modules = [libfdt_module],
       package_dir = {'': objdir},
       py_modules = ["libfdt"],
       )

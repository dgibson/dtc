# Device Tree Compiler and libfdt

The source tree contains the Device Tree Compiler (dtc) toolchain for
working with device tree source and binary files and also libfdt, a
utility library for reading and manipulating the binary format.

dtc and libfdt are maintained by:

* [David Gibson `<david@gibson.dropbear.id.au>`](mailto:david@gibson.dropbear.id.au)

## Building

dtc and libfdt are built with [Meson](https://mesonbuild.com/):

```
$ meson setup build
$ meson compile -C build
$ meson test -C build
$ meson install -C build
```

`meson configure build` lists the build options, e.g. `-Dpython=disabled`
to skip the Python library. See the
[Meson quick guide](https://mesonbuild.com/Quick-guide.html) for more,
such as installing with `--destdir`. `scripts/install-deps.sh` installs
the packages needed on common systems.

Building with `make` still works but is deprecated.

## Python library

A Python library wrapping libfdt is also available. To build this you
will need to install `swig` and Python development files. On Debian
distributions:

```
$ sudo apt-get install swig python3-dev
```

The library provides an `Fdt` class which you can use like this, from
`build/tests` after running the tests:

```
$ PYTHONPATH=../pylibfdt python3
>>> import libfdt
>>> fdt = libfdt.Fdt(open('test_tree1.dtb', mode='rb').read())
>>> node = fdt.path_offset('/subnode@1')
>>> print(node)
124
>>> prop_offset = fdt.first_property_offset(node)
>>> prop = fdt.get_property_by_offset(prop_offset)
>>> print('%s=%s' % (prop.name, prop.as_str()))
compatible=subnode1
>>> node2 = fdt.path_offset('/')
>>> print(fdt.getprop(node2, 'compatible').as_str())
test_tree1
```

You will find tests in `tests/pylibfdt_tests.py` showing how to use each
method. Help is available using the Python help command, e.g.:

```
$ cd build/pylibfdt
$ python3 -c "import libfdt; help(libfdt)"
```

If you add new features, please check code coverage:

```
$ sudo apt-get install python3-coverage
$ cd build/tests
# It's just 'coverage' on most other distributions
$ python3-coverage run ../../tests/pylibfdt_tests.py
$ python3-coverage html
# Open 'htmlcov/index.html' in your browser
```

The library can be installed with pip from a local source tree:

```
$ pip install . [--user|--prefix=/path/to/install_dir]
```

Or directly from a remote git repo:

```
$ pip install git+git://git.kernel.org/pub/scm/utils/dtc/dtc.git@main
```

The install depends on libfdt shared library being installed on the
host system first. Generally, using `--user` or `--prefix` is not
necessary and pip will use the default location for the Python
installation which varies if the user is root or not.

You can also install everything with meson if you like, but pip is
recommended.

To install both libfdt and pylibfdt you can use:

```
$ meson setup [--prefix=/path/to/install_dir] build
$ meson install -C build
```

To disable building the python library, even if swig and Python are available,
use:

```
$ meson setup -Dpython=disabled build
```

More work remains to support all of libfdt, including access to numeric
values.

## dtc

`dtc` compiles device tree source (`.dts`) into flattened device tree
blobs (`.dtb`), decompiles `.dtb` blobs or `/proc/device-tree`-style
filesystem trees (`fs`) back to `.dts`, emits YAML (`.yaml`, when built
with `libyaml`) or assembly (`.S`), and runs semantic checks on the
tree:

```
$ dtc -I dts -O dtb -o board.dtb board.dts
$ dtc -I dtb -O dts -o board.dts board.dtb
```

See `Documentation/manual.txt` for full command-line reference
documentation for `dtc` and the C utility tools below.

## dtdiff

`dtdiff` normalizes and sorts two device trees via `dtc -s -O dts` and
prints a unified diff between them. Each input may be a `.dts` source
file, a `.dtb` / `.dtbo` blob, or a `/proc/device-tree`-style directory:

```
$ dtdiff before.dtb after.dtb
$ dtdiff board.dts /proc/device-tree
```

## fdtget

`fdtget` reads property values (`<node> <property>` pairs), lists
property names (`-p`, `--properties`), or lists subnode names (`-l`,
`--list`) from a `.dtb` blob. Use `-t`, `--type` (e.g. `-ts` for
strings, `-ti` / `-tu` / `-tx` for signed, unsigned, or hex integers, or
`-tr` for raw bytes) to control formatting, and `-d`, `--default` to
provide a fallback when a node or property is missing:

```
$ fdtget board.dtb / model /cpus/cpu@0 reg
$ fdtget -tx board.dtb /soc/serial@1000 reg
$ fdtget -l board.dtb /soc
```

## fdtput

`fdtput` modifies a `.dtb` blob in place: writing property values,
creating nodes (`-c`, `--create`), deleting nodes and their subnodes
(`-r`, `--remove`), or deleting properties (`-d`, `--delete`). Pass
`-t`, `--type` (e.g. `-ts` for strings or `-tx` for hex integers;
default is decimal integers) to control value encoding, and `-p`,
`--auto-path` to create any missing nodes along the target path:

```
$ fdtput -p -ts board.dtb /soc/i2c@3000 compatible vendor,i2c
$ fdtput -tx board.dtb /soc/serial@1000 reg 1000 100
$ fdtput -d board.dtb /soc/i2c@3000 compatible
```

## fdtdump

`fdtdump` is a low-level debugging tool that dumps the FDT header,
`/memreserve/` entries, and raw structure-block contents of a `.dtb`
blob (`-d`, `--debug` prints tag offsets; `-s`, `--scan` scans a larger
file for an embedded FDT). For general decompilation of a `.dtb` to
`.dts`, use `dtc -I dtb -O dts` instead:

```
$ fdtdump -d board.dtb
```

## convert-dtsv0

`convert-dtsv0` converts obsolete version 0 `.dts` source files to
`/dts-v1/;` syntax, writing each output to `<filename>v1`:

```
$ convert-dtsv0 old-board.dts
```

## fdtoverlay

`fdtoverlay` applies one or more compiled `.dtbo` overlays (`-v`,
`--verbose` for progress) onto a base `.dtb` blob (`-i`, `--input`,
compiled with `dtc -@` when overlays reference symbols) and writes the
merged blob to `-o`, `--output`:

```
$ fdtoverlay -i base.dtb -o merged.dtb overlay1.dtbo overlay2.dtbo
```

## Mailing lists

* The [devicetree-compiler](mailto:devicetree-compiler@vger.kernel.org)
  list is for discussion about dtc and libfdt implementation.
* Core device tree bindings are discussed on the
  [devicetree-spec](mailto:devicetree-spec@vger.kernel.org) list.


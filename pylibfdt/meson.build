libfdt_c = custom_target(
  'swig',
  input: 'libfdt.i',
  output: ['libfdt.c', 'libfdt.py'],
  install: true,
  install_dir: [false, py.get_install_dir(pure: false)],
  command: [swig, '-python', '-I'+meson.current_source_dir() / '../libfdt', '-o', '@OUTPUT0@', '@INPUT@']
)

nowarn_gen = cc.get_supported_arguments(
  '-Wno-cast-qual',
  '-Wno-missing-prototypes',
  '-Wno-redundant-decls',
)
pylibfdt = py.extension_module(
  '_libfdt',
  libfdt_c,
  c_args: ['-DPY_SSIZE_T_CLEAN'] + nowarn_gen,
  dependencies: [libfdt_dep, py.dependency()],
  install: true,
)

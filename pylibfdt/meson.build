swig_generator = generator(
  swig,
  output: ['@BASENAME@_wrap.c'],
  arguments: [
    '-python',
    '-I' + join_paths(meson.source_root(), 'libfdt'),
    '-o', join_paths(meson.project_build_root(), '@OUTPUT@'),
    '@INPUT@',
  ]
)

py.extension_module(
  '_libfdt',
  swig_generator.process('libfdt.i'),
  dependencies: [
    py.dependency(),
    libfdt_dep,
  ],
)

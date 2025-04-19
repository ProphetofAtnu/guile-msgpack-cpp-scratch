project('guile-mpack-cpp', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++20'])

msgpack_dep = dependency('msgpack-cxx')
guiledep = dependency('guile-3.0')
boostdep = dependency('boost')

exe = executable('guile-mpack-cpp', 'guile_mpack_cpp.cpp',
  install : true,
  dependencies: [boostdep, msgpack_dep, guiledep])

test('basic', exe)

project('guile-mpack-cpp', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++20'])

msgpackdep = dependency('msgpack-cxx')
guiledep = dependency('guile-3.0')
boostdep = dependency('boost')

extension = shared_library('guile-mpack', 'extension.cpp',
  dependencies: [boostdep, msgpackdep, guiledep])

exe = executable('guile-mpack-cpp', 'guile_mpack_cpp.cpp',
  install : true,
  dependencies: [boostdep, msgpackdep, guiledep])

test('basic', exe)

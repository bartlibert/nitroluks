project('nitro_luks', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++14'])

nitrokey = dependency('libnitrokey-1')
fmt = dependency('fmt', static: true)

includes = include_directories('include')

sources = [
  'src/NitroKey.cpp',
  'src/exceptions.cpp']

exe = executable(
  'nitro_luks',
  [sources, 'src/nitro_luks.cpp'],
  include_directories: includes,
  dependencies: [nitrokey, fmt],
  install : true)

install_data(
  'keyscript.sh',
  install_dir:get_option('bindir'),
  install_mode:'rwxr-xr-x')

install_data(
  'nitroluks_crypttab',
  install_dir:get_option('sysconfdir') / 'nitroluks',
  rename:'nitroluks_crypttab.conf',
  install_mode:'rwxr-xr-x')

install_data(
  'initramfs-hook',
  install_dir:get_option('sysconfdir') / 'initramfs-tools' / 'hooks',
  rename:'nitroluks',
  install_mode:'rwxr-xr-x')

tests = executable(
  'tests',
  sources: [
    sources,
    'test/main.cpp',
    'test/test_nitrokey.cpp'
    ],
  dependencies: [fmt, nitrokey],
  include_directories: [includes, include_directories('test/lib')]
  )
test('nitrolukspp', tests)

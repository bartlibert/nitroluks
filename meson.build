project('nitro_luks', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++14'])

nitrokey = dependency('libnitrokey-1')
fmt = dependency('fmt', static: true)

includes = include_directories([
  'include',
  'third_party/gsl/include'])

sources = [
  'src/nitro_luks.cpp',
  'src/NitroKey.cpp',
  'src/exceptions.cpp']

exe = executable(
  'nitro_luks',
  sources,
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

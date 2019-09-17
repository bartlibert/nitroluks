project('nitro_luks', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3'])

nitrokey = dependency('libnitrokey-1')

sources = ['src/nitro_luks.cpp']

exe = executable(
  'nitro_luks',
  sources,
  dependencies: [nitrokey],
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

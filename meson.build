project(
    'membervars',
    ['c'],
    version: '1.0.0',
    license: 'AGPL-3.0-or-later',
    meson_version: '>=0.56.0',
    default_options: [
        'warning_level=1',
        'c_std=c99',
    ],
)

# Configuration
conf_data = configuration_data()
conf_data.set_quoted('VERSION', meson.project_version())
conf_data.set_quoted('PACKAGE_NAME', meson.project_name())

# Get prefix from options
prefix = get_option('prefix')
bindir = get_option('bindir')
libdir = get_option('libdir')
includedir = get_option('includedir')
mandir = get_option('mandir')
pkgdatadir = get_option('datadir') / meson.project_name()

conf_data.set_quoted('PREFIX', prefix)
conf_data.set_quoted('BINDIR', prefix / bindir)
conf_data.set_quoted('LIBDIR', prefix / libdir)
conf_data.set_quoted('INCLUDEDIR', prefix / includedir)
conf_data.set_quoted('MANDIR', prefix / mandir)

# Generate config.h
configure_file(output: 'config.h', configuration: conf_data)

cc = meson.get_compiler('c')

bas_dep = cc.find_library('bas-c')
exe_deps = [bas_dep]

# Include directories
exe_inc = include_directories(
    '.',
)

membervars_exe = executable(
    'membervars',
    'membervars.c',
    dependencies: exe_deps,
    include_directories: exe_inc,
    c_args: ['-DLOGGER_NAME=membervars'],
    install: true,
)

install_man('man/membervars.1')

install_data(
    'completions/membervars',
    install_dir: get_option('datadir') / 'bash-completion' / 'completions',
)

sh_prog = find_program('sh')

test(
    'membervars-conversion',
    sh_prog,
    args: [
        files('tests/test_membervars.sh'),
        membervars_exe,
        meson.project_source_root(),
    ],
)


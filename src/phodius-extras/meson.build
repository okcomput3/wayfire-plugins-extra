boost = dependency('boost')

phodius_extras = shared_module('phodius-extras', 'plugin.cpp',
    dependencies: [wayfire, boost],
    install: true, install_dir: join_paths(get_option('libdir'), 'wayfire'))

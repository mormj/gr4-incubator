ldpc_proj = subproject('ldpc-toolbox', required: true)
ldpc_dep = ldpc_proj.get_variable('ldpc_toolbox_dep')

inc_dirs = include_directories('include')

deps = [gr4_dep, gr4_blocklib_dep, ldpc_dep, gr4_zmq_blocks_dep]

gr4_packet_modem_blocks_dep = declare_dependency(
  include_directories: [inc_dirs],
  dependencies: deps
)

install_subdir(
  'include/gnuradio-4.0/packet-modem',
  install_dir: join_paths(get_option('includedir'), 'gnuradio-4.0')
)

subdir('apps')
subdir('benchmarks')
subdir('examples')
if get_option('enable_testing')
  subdir('test')
endif

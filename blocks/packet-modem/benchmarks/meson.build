bench_names = [
  'benchmark_packet_receiver',
  'benchmark_packet_transceiver',
]

foreach exe_name : bench_names
  executable(
    exe_name,
    exe_name + '.cpp',
    include_directories: [include_directories('..')],
    dependencies: [gr4_packet_modem_blocks_dep, gr4_zmq_blocks_dep],
  )
endforeach

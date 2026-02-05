app_names = [
  'packet_receiver_file',
  'packet_receiver_soapy',
  'packet_transceiver',
  'packet_transmitter_pdu',
  'packet_transmitter_pdu_throttle',
  'packet_transmitter_sdr',
]

foreach exe_name : app_names
  executable(
    exe_name,
    exe_name + '.cpp',
    include_directories: [include_directories('..')],
    dependencies: [gr4_packet_modem_blocks_dep, gr4_zmq_blocks_dep, gr4_soapysdr_blocks_dep],
  )
endforeach

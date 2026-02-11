example_names = [
  # 'crc_append_missing_tags',
  # 'crc_check',
  # 'crc',
  # 'done_msg_sink',
  # 'head',
  # 'header_formatter',
  # 'messages',
  # 'minimal_flowgraph',
  # 'multithreaded_test2',
  # 'multithreaded_test',
  # 'pack_bits',
  # 'packet_ingress',
  # 'packet_mux_benchmark',
  # 'packet_strobe',
  # 'packet_to_stream',
  # 'packet_transmitter',
  # 'packet_transmitter_performance',
  # 'pdu_specialization',
  # 'pdu_to_tagged_stream',
  # 'pfb_arb_resampler',
  # 'qpsk_modulator',
  # 'stop_flowgraph',
  # 'syncword_detection',
  # 'tagged_stream_to_pdu',
  # 'tags',
  # 'test_required_samples',
  # 'tun',
]

foreach exe_name : example_names
  executable(
    exe_name,
    exe_name + '.cpp',
    include_directories: [include_directories('..')],
    dependencies: [gr4_packet_modem_blocks_dep, gr4_zmq_blocks_dep],
  )
endforeach

tests = []
if ut_dep.found()
  tests += [
    # ['qa_add', ['qa_add.cpp']],
    # ['qa_additive_scrambler', ['qa_additive_scrambler.cpp']],
    # ['qa_binary_slicer', ['qa_binary_slicer.cpp']],
    # ['qa_burst_shaper', ['qa_burst_shaper.cpp']],
    # ['qa_coarse_frequency_correction', ['qa_coarse_frequency_correction.cpp']],
    # ['qa_constellation_llr_decoder', ['qa_constellation_llr_decoder.cpp']],
    # ['qa_costas_loop', ['qa_costas_loop.cpp']],
    # ['qa_crc', ['qa_crc.cpp']],
    # ['qa_firdes', ['qa_firdes.cpp']],
    # ['qa_glfsr_source', ['qa_glfsr_source.cpp']],
    # ['qa_head', ['qa_head.cpp']],
    # ['qa_header_fec_decoder', ['qa_header_fec_decoder.cpp']],
    # ['qa_header_fec_encoder', ['qa_header_fec_encoder.cpp']],
    # ['qa_header_formatter', ['qa_header_formatter.cpp']],
    # ['qa_header_parser', ['qa_header_parser.cpp']],
    # ['qa_header_payload_split', ['qa_header_payload_split.cpp']],
    # ['qa_interpolating_fir_filter', ['qa_interpolating_fir_filter.cpp']],
    # ['qa_item_strobe', ['qa_item_strobe.cpp']],
    # ['qa_loopback', ['qa_loopback.cpp']],
    # ['qa_mapper', ['qa_mapper.cpp']],
    # ['qa_messages', ['qa_messages.cpp']],
    # ['qa_multiply_packet_len_tag', ['qa_multiply_packet_len_tag.cpp']],
    # ['qa_noise_source', ['qa_noise_source.cpp']],
    # ['qa_null_source_and_sink', ['qa_null_source_and_sink.cpp']],
    # ['qa_pack_unpack', ['qa_pack_unpack.cpp']],
    # ['qa_packet_ingress', ['qa_packet_ingress.cpp']],
    # ['qa_packet_mux', ['qa_packet_mux.cpp']],
    # ['qa_packet_strobe', ['qa_packet_strobe.cpp']],
    # ['qa_packet_to_stream', ['qa_packet_to_stream.cpp']],
    # ['qa_payload_metadata_insert', ['qa_payload_metadata_insert.cpp']],
    # ['qa_pdu', ['qa_pdu.cpp']],
    # ['qa_pfb_arb_resampler', ['qa_pfb_arb_resampler.cpp']],
    # ['qa_probe_rate', ['qa_probe_rate.cpp']],
    # ['qa_rotator', ['qa_rotator.cpp']],
    # ['qa_stream_to_tagged_stream', ['qa_stream_to_tagged_stream.cpp']],
    # ['qa_symbol_filter', ['qa_symbol_filter.cpp']],
    # ['qa_syncword_detection', ['qa_syncword_detection.cpp']],
    # ['qa_syncword_detection_filter', ['qa_syncword_detection_filter.cpp']],
    # ['qa_syncword_remove', ['qa_syncword_remove.cpp']],
    # ['qa_syncword_wipeoff', ['qa_syncword_wipeoff.cpp']],
    # ['qa_tag_gate', ['qa_tag_gate.cpp']],
    # ['qa_throttle', ['qa_throttle.cpp']],
    ['qa_vector_source_and_sink', ['qa_vector_source_and_sink.cpp']],
  ]
else
  message('Skipping tests: boost-ut not found')
endif

foreach test_info : tests
  test_name = test_info[0]
  test_sources = test_info[1]

  exe = executable(
    test_name,
    test_sources,
    include_directories: [include_directories('..', '../include')],
    dependencies: [gr4_packet_modem_blocks_dep, gr4_zmq_blocks_dep, ut_dep],
  )

  test(test_name, exe)
endforeach

ut_dep = dependency('boost-ut', required: false)

tests = []
if ut_dep.found()
  tests += [
    ['qa_crc', ['qa_crc.cpp']],
    ['qa_loopback', ['qa_loopback.cpp']],
    ['qa_packet_transceiver', ['qa_packet_transceiver.cpp']],
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
    include_directories: [include_directories('..')],
    dependencies: [gr4_packet_modem_blocks_dep, gr4_zmq_blocks_dep, ut_dep],
  )

  test(test_name, exe)
endforeach

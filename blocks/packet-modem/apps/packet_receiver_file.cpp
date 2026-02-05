#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/packet-modem/scheduler_helpers.hpp>
#include <gnuradio-4.0/packet-modem/file_source.hpp>
#include <gnuradio-4.0/packet-modem/packet_receiver.hpp>
#include <gnuradio-4.0/packet-modem/packet_type_filter.hpp>
#include <gnuradio-4.0/packet-modem/tagged_stream_to_pdu.hpp>
#include <gnuradio-4.0/packet-modem/tun_sink.hpp>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>
#include <gnuradio-4.0/soapy/Soapy.hpp>
#include <complex>
#include <cstdint>
#include <cstdlib>

#include <print>
int main(int argc, char** argv)
{
    using c64 = std::complex<float>;

    if ((argc < 2) || (argc > 4)) {
        std::println(stderr,
                     "usage: {} input_file [syncword_freq_bins] [syncword_threshold]",
                     argv[0]);
        std::println(stderr, "");
        std::println(stderr, "the default syncword freq bins is 4");
        std::println(stderr, "the default syncword threshold is 9.5");
        std::exit(1);
    }
    const int syncword_freq_bins = argc >= 3 ? std::stoi(argv[2]) : 4;
    const float syncword_threshold = argc >= 4 ? std::stof(argv[3]) : 9.5f;

    gr::Graph fg;
    auto& file_source =
        fg.emplaceBlock<gr::packet_modem::FileSource<c64>>(
            gr::packet_modem::make_props({ { "filename", gr::packet_modem::pmt_value(argv[1]) } }));
    const size_t samples_per_symbol = 4;
    const bool header_debug = false;
    const bool zmq_output = true;
    const bool log = true;
    auto packet_receiver = gr::packet_modem::PacketReceiver(fg,
                                                            samples_per_symbol,
                                                            "packet_len",
                                                            header_debug,
                                                            zmq_output,
                                                            log,
                                                            syncword_freq_bins,
                                                            syncword_threshold);
    auto& packet_type_filter = fg.emplaceBlock<gr::packet_modem::PacketTypeFilter<>>(
        gr::packet_modem::make_props({ { "packet_type", gr::packet_modem::pmt_value("user_data") } }));
    auto& tag_to_pdu = fg.emplaceBlock<gr::packet_modem::TaggedStreamToPdu<uint8_t>>();
    auto& sink = fg.emplaceBlock<gr::packet_modem::TunSink>(
        gr::packet_modem::make_props({ { "tun_name", gr::packet_modem::pmt_value("gr4_tun_rx") },
                                       { "netns_name", gr::packet_modem::pmt_value("gr4_rx") } }));

    const char* connection_error = "connection error";

    if (fg.connect<"out">(file_source).to<"in">(*packet_receiver.syncword_detection) !=
        gr::ConnectionResult::SUCCESS) {
        throw gr::exception(connection_error);
    }
    if (fg.connect<"out">(*packet_receiver.payload_crc_check)
            .to<"in">(packet_type_filter) != gr::ConnectionResult::SUCCESS) {
        throw gr::exception(connection_error);
    }
    if (fg.connect<"out">(packet_type_filter).to<"in">(tag_to_pdu) !=
        gr::ConnectionResult::SUCCESS) {
        throw gr::exception(connection_error);
    }
    if (fg.connect<"out">(tag_to_pdu).to<"in">(sink) != gr::ConnectionResult::SUCCESS) {
        throw gr::exception(connection_error);
    }

    gr::scheduler::Simple<gr::scheduler::ExecutionPolicy::singleThreaded> sched;
    gr::packet_modem::init_scheduler(sched, std::move(fg));
    const auto ret = sched.runAndWait();
    if (!ret.has_value()) {
        std::println(stderr, "scheduler error: {}", ret.error());
        std::exit(1);
    }

    return 0;
}

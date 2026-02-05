#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/packet-modem/scheduler_helpers.hpp>
#include <gnuradio-4.0/packet-modem/payload_metadata_insert.hpp>
#include <gnuradio-4.0/packet-modem/vector_sink.hpp>
#include <gnuradio-4.0/packet-modem/vector_source.hpp>
#include <boost/ut.hpp>
#include <complex>

#include <print>
boost::ut::suite PayloadMetadataInsertTests = [] {
    using namespace boost::ut;
    using namespace gr;
    using namespace gr::packet_modem;

    "payload_metadata_insert_header_only"_test = [] {
        Graph fg;
        const size_t num_items = 100000;
        // although in principle it should be possible to instantiate
        // PayloadMetadataInsert<int>, doing so gives compiler errors
        using c64 = std::complex<float>;
        std::vector<c64> v(num_items);
        std::iota(v.begin(), v.end(), 0);
        const size_t syncword_index = 12345;
        const std::vector<Tag> tags = { { static_cast<ssize_t>(syncword_index),
                                          make_props({ { "syncword_amplitude", pmt_value(0.1f) } }) } };
        const size_t syncword_size = 64;
        const size_t header_size = 128;
        auto& source = fg.emplaceBlock<VectorSource<c64>>();
        source.data = v;
        source.tags = tags;
        auto& payload_metadata_insert = fg.emplaceBlock<PayloadMetadataInsert<>>(
            make_props({ { "syncword_size", pmt_value(syncword_size) }, { "header_size", pmt_value(header_size) } }));
        auto& sink = fg.emplaceBlock<VectorSink<c64>>();
        expect(eq(ConnectionResult::SUCCESS,
                  fg.connect<"out">(source).to<"in">(payload_metadata_insert)));
        expect(eq(ConnectionResult::SUCCESS,
                  fg.connect<"out">(payload_metadata_insert).to<"in">(sink)));
        scheduler::Simple sched;
        gr::packet_modem::init_scheduler(sched, std::move(fg));
        // this test needs to be stopped with a message, because the
        // PayloadMetadataInsert keeps waiting for a message containing the
        // header decode
        MsgPortOut toScheduler;
        expect(eq(ConnectionResult::SUCCESS, toScheduler.connect(sched.msgIn)));
        std::thread stopper([&toScheduler]() {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            sendMessage<message::Command::Set>(toScheduler,
                                               "",
                                               block::property::kLifeCycleState,
                                               { { "state", "REQUESTED_STOP" } });
        });
        expect(sched.runAndWait().has_value());
        stopper.join();
        const auto data = sink.data();
        expect(eq(data.size(), syncword_size + header_size));
        for (size_t j = 0; j < data.size(); ++j) {
            expect(eq(data[j], v[syncword_index + j]));
        }
        const auto sink_tags = sink.tags();
        expect(eq(sink_tags.size(), 2_ul));
        const auto& syncword_tag = sink_tags.at(0);
        expect(eq(syncword_tag.index, 0_l));
        const auto syncword_constellation = find_prop(syncword_tag.map, "constellation");
        expect(syncword_constellation != syncword_tag.map.end());
        expect(eq(gr::packet_modem::pmt_cast<std::string>(syncword_constellation->second),
                  std::string("PILOT")));
        expect(find_prop(syncword_tag.map, "loop_bandwidth") != syncword_tag.map.end());
        const auto syncword_amplitude = find_prop(syncword_tag.map, "syncword_amplitude");
        expect(syncword_amplitude != syncword_tag.map.end());
        expect(eq(gr::packet_modem::pmt_cast<float>(syncword_amplitude->second), 0.1f));
        const auto& header_tag = sink_tags.at(1);
        expect(eq(header_tag.index, static_cast<ssize_t>(syncword_size)));
        const auto header_constellation = find_prop(header_tag.map, "constellation");
        expect(header_constellation != header_tag.map.end());
        expect(eq(gr::packet_modem::pmt_cast<std::string>(header_constellation->second),
                  std::string("QPSK")));
        const auto header_start = find_prop(header_tag.map, "header_start");
        expect(header_start != header_tag.map.end());
        expect(header_start->second == gr::packet_modem::pmt_null());
        expect(find_prop(header_tag.map, "loop_bandwidth") != header_tag.map.end());
    };

    "payload_metadata_insert_with_payload"_test = [] {
        Graph fg;
        const size_t num_items = 100000;
        // although in principle it should be possible to instantiate
        // PayloadMetadataInsert<int>, doing so gives compiler errors
        using c64 = std::complex<float>;
        std::vector<c64> v(num_items);
        std::iota(v.begin(), v.end(), 0);
        const size_t syncword_index = 12345;
        const std::vector<Tag> tags = { { static_cast<ssize_t>(syncword_index),
                                          make_props({ { "syncword_amplitude", pmt_value(0.1f) } }) } };
        const size_t syncword_size = 64;
        const size_t header_size = 128;
        auto& source = fg.emplaceBlock<VectorSource<c64>>();
        source.data = v;
        source.tags = tags;
        auto& payload_metadata_insert = fg.emplaceBlock<PayloadMetadataInsert<>>(
            make_props({ { "syncword_size", pmt_value(syncword_size) }, { "header_size", pmt_value(header_size) } }));
        auto& sink = fg.emplaceBlock<VectorSink<c64>>();
        // repeat set to true in this block because otherwise it says it's DONE
        // and this causes the flowgraph to terminate
        auto& parsed_source =
            fg.emplaceBlock<VectorSource<Message>>(make_props({ { "repeat", pmt_value(true) } }));
        const size_t packet_length = 100;
        // +32 because of the CRC-32
        const size_t payload_bits = 8 * packet_length + 32;
        const size_t payload_symbols = payload_bits / 2;
        Message parsed;
        parsed.data = make_props({ { "packet_length", pmt_value(packet_length) } });
        parsed_source.data = std::vector<Message>{ std::move(parsed) };
        expect(eq(ConnectionResult::SUCCESS,
                  fg.connect<"out">(source).to<"in">(payload_metadata_insert)));
        expect(eq(ConnectionResult::SUCCESS,
                  fg.connect<"out">(payload_metadata_insert).to<"in">(sink)));
        expect(eq(ConnectionResult::SUCCESS,
                  fg.connect<"out">(parsed_source)
                      .to<"parsed_header">(payload_metadata_insert)));
        scheduler::Simple sched;
        gr::packet_modem::init_scheduler(sched, std::move(fg));
        expect(sched.runAndWait().has_value());
        const auto data = sink.data();
        expect(eq(data.size(), syncword_size + header_size + payload_symbols));
        for (size_t j = 0; j < data.size(); ++j) {
            expect(eq(data[j], v[syncword_index + j]));
        }
        const auto sink_tags = sink.tags();
        for (const auto& tag : sink_tags) {
            std::println("{} {}", tag.index, tag.map);
        }
        expect(eq(sink_tags.size(), 3_ul));
        const auto& syncword_tag = sink_tags.at(0);
        expect(eq(syncword_tag.index, 0_l));
        const auto syncword_constellation = find_prop(syncword_tag.map, "constellation");
        expect(syncword_constellation != syncword_tag.map.end());
        expect(eq(gr::packet_modem::pmt_cast<std::string>(syncword_constellation->second),
                  std::string("PILOT")));
        expect(find_prop(syncword_tag.map, "loop_bandwidth") != syncword_tag.map.end());
        const auto syncword_amplitude = find_prop(syncword_tag.map, "syncword_amplitude");
        expect(syncword_amplitude != syncword_tag.map.end());
        expect(eq(gr::packet_modem::pmt_cast<float>(syncword_amplitude->second), 0.1f));
        const auto& header_tag = sink_tags.at(1);
        expect(eq(header_tag.index, static_cast<ssize_t>(syncword_size)));
        const auto header_constellation = find_prop(header_tag.map, "constellation");
        expect(header_constellation != header_tag.map.end());
        expect(eq(gr::packet_modem::pmt_cast<std::string>(header_constellation->second),
                  std::string("QPSK")));
        const auto header_start = find_prop(header_tag.map, "header_start");
        expect(header_start != header_tag.map.end());
        expect(header_start->second == gr::packet_modem::pmt_null());
        expect(find_prop(header_tag.map, "loop_bandwidth") != header_tag.map.end());
        const auto& payload_tag = sink_tags.at(2);
        expect(eq(payload_tag.index, static_cast<ssize_t>(syncword_size + header_size)));
        const auto payload_bits_it = find_prop(payload_tag.map, "payload_bits");
        expect(payload_bits_it != payload_tag.map.end());
        expect(eq(gr::packet_modem::pmt_cast<size_t>(payload_bits_it->second), payload_bits));
        const auto payload_symbols_it = find_prop(payload_tag.map, "payload_symbols");
        expect(payload_symbols_it != payload_tag.map.end());
        expect(eq(gr::packet_modem::pmt_cast<size_t>(payload_symbols_it->second),
                  payload_symbols));
        expect(find_prop(payload_tag.map, "constellation") == payload_tag.map.end());
        expect(find_prop(payload_tag.map, "loop_bandwidth") != payload_tag.map.end());
    };
};

int main() {}

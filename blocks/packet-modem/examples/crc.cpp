#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/packet-modem/scheduler_helpers.hpp>
#include <gnuradio-4.0/packet-modem/crc.hpp>
#include <gnuradio-4.0/packet-modem/crc_append.hpp>
#include <gnuradio-4.0/packet-modem/packet_strobe.hpp>
#include <gnuradio-4.0/packet-modem/vector_sink.hpp>
#include <gnuradio-4.0/packet-modem/vector_source.hpp>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>
#include <boost/ut.hpp>
#include <numeric>

#include <print>
int main()
{
    using namespace boost::ut;

    gr::Graph fg;

    std::vector<uint8_t> data(10);
    const std::vector<gr::Tag> tags = {
        { 0, gr::packet_modem::make_props({ { "packet_len", 8U } }) },
        { 1, gr::packet_modem::make_props({ { "foo", "bar" } }) },
        { 8, gr::packet_modem::make_props({ { "packet_len", 2U } }) },
    };
    // Uncomment just one of these two sources
    //

    auto& source = fg.emplaceBlock<gr::packet_modem::VectorSource<uint8_t>>();
    source.data = data;
    source.tags = tags;

    // auto& source = fg.emplaceBlock<gr::packet_modem::PacketStrobe<uint8_t>>(
    //     8U, std::chrono::seconds(3), "packet_len", true);

    //

    auto& crc_append = fg.emplaceBlock<gr::packet_modem::CrcAppend<>>(
        gr::packet_modem::make_props({
            { "num_bits", 16U },
            { "poly", uint64_t{ 0x1021 } },
            { "initial_value", uint64_t{ 0xFFFF } },
            { "final_xor", uint64_t{ 0xFFFF } },
            { "input_reflected", true },
            { "result_reflected", true },
        }));
    auto& sink = fg.emplaceBlock<gr::packet_modem::VectorSink<uint8_t>>();
    expect(eq(gr::ConnectionResult::SUCCESS,
              fg.connect<"out">(source).to<"in">(crc_append)));
    expect(
        eq(gr::ConnectionResult::SUCCESS, fg.connect<"out">(crc_append).to<"in">(sink)));

    gr::scheduler::Simple sched;
    gr::packet_modem::init_scheduler(sched, std::move(fg));
    expect(sched.runAndWait().has_value());

    const auto sink_data = sink.data();
    std::print("vector sink contains {} items\n", sink_data.size());
    std::print("vector sink items:\n");
    for (const auto n : sink_data) {
        std::print("0x{:02x} ", n);
    }
    std::print("\n");
    std::print("vector sink tags:\n");
    const auto sink_tags = sink.tags();
    for (const auto& t : sink_tags) {
        std::print("index = {}, map = {}\n", t.index, t.map);
    }

    return 0;
}

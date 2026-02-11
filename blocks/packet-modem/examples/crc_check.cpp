#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/packet-modem/scheduler_helpers.hpp>
#include <gnuradio-4.0/packet-modem/crc_append.hpp>
#include <gnuradio-4.0/packet-modem/crc_check.hpp>
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
        //{ 1, { { "foo", "bar" } } },
        { 8, gr::packet_modem::make_props({ { "packet_len", 2U } }) },
    };
    auto& source =
        fg.emplaceBlock<gr::packet_modem::VectorSource<uint8_t>>(
            gr::packet_modem::make_props({ { "repeat", false } }));
    source.data = data;
    source.tags = tags;
    auto& crc_append = fg.emplaceBlock<gr::packet_modem::CrcAppend<>>(
        gr::packet_modem::make_props({
            { "num_bits", 16U },
            { "poly", uint64_t{ 0x1021 } },
            { "initial_value", uint64_t{ 0xFFFF } },
            { "final_xor", uint64_t{ 0xFFFF } },
            { "input_reflected", true },
            { "result_reflected", true },
        }));
    auto& crc_check = fg.emplaceBlock<gr::packet_modem::CrcCheck<>>(
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
    expect(eq(gr::ConnectionResult::SUCCESS,
              fg.connect<"out">(crc_append).to<"in">(crc_check)));
    expect(
        eq(gr::ConnectionResult::SUCCESS, fg.connect<"out">(crc_check).to<"in">(sink)));

    gr::scheduler::Simple sched;
    gr::packet_modem::init_scheduler(sched, std::move(fg));

    gr::MsgPortOut toScheduler;
    expect(eq(gr::ConnectionResult::SUCCESS, toScheduler.connect(sched.msgIn)));
    std::thread stopper([&toScheduler]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        gr::sendMessage<gr::message::Command::Set>(
            toScheduler,
            "",
            gr::block::property::kLifeCycleState,
            gr::packet_modem::make_props(
                { { "state", "REQUESTED_STOP" } }));
    });

    const auto ret = sched.runAndWait();
    stopper.join();
    if (!ret.has_value()) {
        std::print("ret = {}\n", ret.error());
    }
    expect(ret.has_value());

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

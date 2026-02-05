#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/packet-modem/scheduler_helpers.hpp>
#include <gnuradio-4.0/packet-modem/packet_strobe.hpp>
#include <gnuradio-4.0/packet-modem/vector_sink.hpp>
#include <gnuradio-4.0/packet-modem/vector_source.hpp>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>
#include <boost/ut.hpp>
#include <chrono>
#include <numeric>
#include <thread>

#include <print>
int main()
{
    using namespace boost::ut;

    gr::Graph fg;

    auto& source = fg.emplaceBlock<gr::packet_modem::PacketStrobe<int>>(
        { { "packet_len", uint64_t{ 25 } },
          { "interval_secs", 3.0 },
          { "packet_len_tag_key", "packet_len" } });
    auto& sink = fg.emplaceBlock<gr::packet_modem::VectorSink<int>>();
    expect(eq(gr::ConnectionResult::SUCCESS, fg.connect<"out">(source).to<"in">(sink)));

    gr::scheduler::Simple sched;
    gr::packet_modem::init_scheduler(sched, std::move(fg));
    gr::MsgPortOut toScheduler;
    expect(eq(gr::ConnectionResult::SUCCESS, toScheduler.connect(sched.msgIn)));

    std::thread stopper([&toScheduler]() {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        gr::sendMessage<gr::message::Command::Set>(toScheduler,
                                                   "",
                                                   gr::block::property::kLifeCycleState,
                                                   { { "state", "REQUESTED_STOP" } });
    });
    expect(sched.runAndWait().has_value());
    stopper.join();

    const auto data = sink.data();
    std::print("vector sink contains {} items\n", data.size());
    std::print("vector sink tags:\n");
    const auto sink_tags = sink.tags();
    for (const auto& t : sink_tags) {
        std::print("index = {}, map = {}\n", t.index, t.map);
    }

    return 0;
}

#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/packet-modem/scheduler_helpers.hpp>
#include <gnuradio-4.0/packet-modem/message_debug.hpp>
#include <gnuradio-4.0/packet-modem/packet_ingress.hpp>
#include <gnuradio-4.0/packet-modem/packet_strobe.hpp>
#include <gnuradio-4.0/packet-modem/vector_sink.hpp>
#include <gnuradio-4.0/packet-modem/vector_source.hpp>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>
#include <boost/ut.hpp>
#include <thread>

#include <print>
int main()
{
    using namespace boost::ut;

    gr::Graph fg;

    const std::vector<size_t> packet_lengths = { 10,     23, 2,     1024, 7,
                                                 100000, 45, 51234, 28,   100 };
    size_t offset = 0;
    std::vector<gr::Tag> tags;
    for (const auto len : packet_lengths) {
        gr::property_map props =
            gr::packet_modem::make_props({ { "packet_len", gr::packet_modem::pmt_value(len) } });
        if (len > std::numeric_limits<uint16_t>::max()) {
            gr::packet_modem::set_prop(props, "invalid", gr::packet_modem::pmt_value(true));
        }
        tags.push_back({ static_cast<ssize_t>(offset), props });
        if (len > 50) {
            tags.push_back({ static_cast<ssize_t>(offset + 40),
                             gr::packet_modem::make_props(
                                 { { "packet_rest", gr::packet_modem::pmt_value(true) } }) });
        }
        offset += len;
    }
    std::vector<uint8_t> v(offset);
    auto& packet_source = fg.emplaceBlock<gr::packet_modem::VectorSource<uint8_t>>();
    packet_source.data = v;
    packet_source.tags = tags;

    auto& ingress = fg.emplaceBlock<gr::packet_modem::PacketIngress<>>();
    auto& sink = fg.emplaceBlock<gr::packet_modem::VectorSink<uint8_t>>();
    auto& meta_sink = fg.emplaceBlock<gr::packet_modem::VectorSink<gr::Message>>();
    expect(eq(gr::ConnectionResult::SUCCESS,
              fg.connect<"out">(packet_source).to<"in">(ingress)));
    expect(eq(gr::ConnectionResult::SUCCESS, fg.connect<"out">(ingress).to<"in">(sink)));
    expect(eq(gr::ConnectionResult::SUCCESS,
              fg.connect<"metadata">(ingress).to<"in">(meta_sink)));

    gr::scheduler::Simple sched;
    gr::packet_modem::init_scheduler(sched, std::move(fg));
    gr::MsgPortOut toScheduler;
    expect(eq(gr::ConnectionResult::SUCCESS, toScheduler.connect(sched.msgIn)));

    std::thread stopper([&toScheduler]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::print("sending REQUEST_STOP to scheduler\n");
        gr::sendMessage<gr::message::Command::Set>(
            toScheduler,
            "",
            gr::block::property::kLifeCycleState,
            gr::packet_modem::make_props({ { "state", gr::packet_modem::pmt_value("REQUESTED_STOP") } }));
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
    const auto messages = meta_sink.data();
    std::println("messages vector sink elements:");
    for (const auto& msg : messages) {
        std::println("{}", msg.data);
    }

    return 0;
}

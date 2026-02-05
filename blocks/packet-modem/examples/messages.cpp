#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/packet-modem/scheduler_helpers.hpp>
#include <gnuradio-4.0/packet-modem/message_debug.hpp>
#include <gnuradio-4.0/packet-modem/message_strobe.hpp>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>
#include <boost/ut.hpp>
#include <thread>

#include <print>
int main()
{
    using namespace boost::ut;

    gr::Graph fg;

    const gr::property_map message =
        gr::packet_modem::make_props({ { "test", gr::packet_modem::pmt_value(1) } });
    auto& strobe = fg.emplaceBlock<gr::packet_modem::MessageStrobe<>>(
        gr::packet_modem::make_props({ { "message", gr::packet_modem::pmt_value(message) },
                                       { "interval_secs", gr::packet_modem::pmt_value(1.0) } }));
    auto& debug = fg.emplaceBlock<gr::packet_modem::MessageDebug>();
    expect(eq(gr::ConnectionResult::SUCCESS,
              fg.connect<"strobe">(strobe).to<"print">(debug)));
    expect(eq(gr::ConnectionResult::SUCCESS,
              fg.connect<"strobe">(strobe).to<"store">(debug)));

    gr::scheduler::Simple sched;
    gr::packet_modem::init_scheduler(sched, std::move(fg));
    gr::MsgPortOut toScheduler;
    expect(eq(gr::ConnectionResult::SUCCESS, toScheduler.connect(sched.msgIn)));

    std::thread stopper([&toScheduler]() {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::print("sending REQUEST_STOP to scheduler\n");
        gr::sendMessage<gr::message::Command::Set>(
            toScheduler,
            "",
            gr::block::property::kLifeCycleState,
            gr::packet_modem::make_props({ { "state", gr::packet_modem::pmt_value("REQUESTED_STOP") } }));
    });

    std::println("running for 10 seconds...");
    expect(sched.runAndWait().has_value());
    stopper.join();

    std::print("MessageDebug stored these messages:\n");
    for (const auto& m : debug.messages()) {
        std::print("{}\n", m);
    }

    return 0;
}

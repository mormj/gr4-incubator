#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/packet-modem/scheduler_helpers.hpp>
#include <gnuradio-4.0/packet-modem/tagged_stream_to_pdu.hpp>
#include <gnuradio-4.0/packet-modem/vector_sink.hpp>
#include <gnuradio-4.0/packet-modem/vector_source.hpp>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>
#include <boost/ut.hpp>
#include <complex>
#include <numeric>

#include <print>
int main()
{
    using namespace boost::ut;

    gr::Graph fg;

    std::vector<int> v(30);
    std::iota(v.begin(), v.end(), 0);
    const std::vector<gr::Tag> tags = {
        { 0, gr::packet_modem::make_props({ { "packet_len", gr::packet_modem::pmt_value(10U) } }) },
        { 3, gr::packet_modem::make_props({ { "foo", gr::packet_modem::pmt_value("bar") } }) },
        { 10, gr::packet_modem::make_props({ { "packet_len", gr::packet_modem::pmt_value(20U) } }) },
    };
    auto& source = fg.emplaceBlock<gr::packet_modem::VectorSource<int>>();
    source.data = v;
    source.tags = tags;
    auto& stream_to_pdu = fg.emplaceBlock<gr::packet_modem::TaggedStreamToPdu<int>>();
    auto& sink =
        fg.emplaceBlock<gr::packet_modem::VectorSink<gr::packet_modem::Pdu<int>>>();
    expect(eq(gr::ConnectionResult::SUCCESS,
              fg.connect<"out">(source).to<"in">(stream_to_pdu)));
    expect(eq(gr::ConnectionResult::SUCCESS,
              fg.connect<"out">(stream_to_pdu).to<"in">(sink)));

    gr::scheduler::Simple sched;
    gr::packet_modem::init_scheduler(sched, std::move(fg));
    expect(sched.runAndWait().has_value());

    const auto data = sink.data();
    std::print("vector sink contains {} items\n", data.size());
    std::print("vector sink items:\n");
    for (const auto& pdu : data) {
        std::println("data = {}", pdu.data);
        std::println("tags:");
        for (const auto& t : pdu.tags) {
            std::println("index = {}, map = {}", t.index, t.map);
        }
    }
    std::print("\n");
    std::print("vector sink tags:\n");
    const auto sink_tags = sink.tags();
    for (const auto& t : sink_tags) {
        std::print("index = {}, map = {}\n", t.index, t.map);
    }

    return 0;
}

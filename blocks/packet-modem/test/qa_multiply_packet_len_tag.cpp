#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/packet-modem/scheduler_helpers.hpp>
#include <gnuradio-4.0/packet-modem/multiply_packet_len_tag.hpp>
#include <gnuradio-4.0/packet-modem/vector_sink.hpp>
#include <gnuradio-4.0/packet-modem/vector_source.hpp>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>
#include <boost/ut.hpp>

boost::ut::suite MultiplyPacketLenTagTests = [] {
    using namespace boost::ut;
    using namespace gr;
    using namespace gr::packet_modem;

    "multiply_packet_len_tag"_test = [] {
        Graph fg;
        const std::vector<Tag> tags = {
            { 0, make_props({ { "packet_len", pmt_value(7UZ) } }) },
            { 7, make_props({ { "packet_len", pmt_value(23UZ) } }) }
        };
        std::vector<int> v(30);
        std::iota(v.begin(), v.end(), 0);
        auto& source = fg.emplaceBlock<VectorSource<int>>();
        source.data = v;
        source.tags = tags;
        auto& fec = fg.emplaceBlock<MultiplyPacketLenTag<int>>(
            make_props({ { "mult", pmt_value(5.0) } }));
        auto& sink = fg.emplaceBlock<VectorSink<int>>();
        expect(eq(ConnectionResult::SUCCESS, fg.connect<"out">(source).to<"in">(fec)));
        expect(eq(ConnectionResult::SUCCESS, fg.connect<"out">(fec).to<"in">(sink)));
        scheduler::Simple sched;
        gr::packet_modem::init_scheduler(sched, std::move(fg));
        expect(sched.runAndWait().has_value());
        expect(eq(sink.data(), v));
        const std::vector<Tag> expected_tags = {
            { 0, make_props({ { "packet_len", pmt_value(35UZ) } }) },
            { 7, make_props({ { "packet_len", pmt_value(115UZ) } }) }
        };
        expect(sink.tags() == expected_tags);
    };
};

int main() {}

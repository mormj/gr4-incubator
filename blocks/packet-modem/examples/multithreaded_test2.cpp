#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/packet-modem/scheduler_helpers.hpp>
#include <gnuradio-4.0/packet-modem/null_sink.hpp>
#include <gnuradio-4.0/packet-modem/null_source.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <boost/ut.hpp>
#include <complex>

#include <print>
template <typename T>
class TestBlock : public gr::Block<TestBlock<T>>
{
public:
    gr::PortIn<T, gr::Async> in;
    gr::PortOut<T, gr::Async> out;

    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan,
                                 ::gr::OutputSpanLike auto& outSpan)
    {
        std::println("{}::processBulk(inSpan.size() = {}, outSpan.size = {})",
                     this->name,
                     inSpan.size(),
                     outSpan.size());
        const size_t n = std::min(inSpan.size(), outSpan.size());
        std::copy_n(inSpan.begin(), n, outSpan.begin());
        if (!inSpan.consume(n)) {
            throw gr::exception("consume failed");
        }
        outSpan.publish(n);
        return gr::work::Status::OK;
    }

    GR_MAKE_REFLECTABLE(TestBlock, in, out);
};

int main(int argc, char* argv[])
{
    using namespace boost::ut;
    using namespace gr::packet_modem;
    using T = int;

    if (argc != 2) {
        std::println(stderr, "usage: {} use_multithreaded", argv[0]);
        std::exit(1);
    }

    const bool use_multithreaded = std::stoi(argv[1]);

    gr::Graph fg;

    auto& source = fg.emplaceBlock<NullSource<T>>();
    auto& test_block = fg.emplaceBlock<TestBlock<T>>();
    auto& sink = fg.emplaceBlock<NullSink<T>>();

    expect(fatal(eq(gr::ConnectionResult::SUCCESS,
                    fg.connect<"out">(source).to<"in">(test_block))));
    expect(fatal(
        eq(gr::ConnectionResult::SUCCESS, fg.connect<"out">(test_block).to<"in">(sink))));

    if (use_multithreaded) {
        std::println(
            "using gr::scheduler::Simple<gr::scheduler::ExecutionPolicy::multiThreaded>");
        gr::scheduler::Simple<gr::scheduler::ExecutionPolicy::multiThreaded> sched;
        gr::packet_modem::init_scheduler(sched, std::move(fg));
        const auto ret = sched.runAndWait();
        expect(ret.has_value());
        if (!ret.has_value()) {
            std::println("scheduler error: {}", ret.error());
        }
    } else {
        std::println(
            "using "
            "gr::scheduler::Simple<gr::scheduler::ExecutionPolicy::singleThreaded>");
        gr::scheduler::Simple<gr::scheduler::ExecutionPolicy::singleThreaded> sched;
        gr::packet_modem::init_scheduler(sched, std::move(fg));
        const auto ret = sched.runAndWait();
        expect(ret.has_value());
        if (!ret.has_value()) {
            std::println("scheduler error: {}", ret.error());
        }
    }

    return 0;
}

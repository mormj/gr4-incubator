#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/packet-modem/scheduler_helpers.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <boost/ut.hpp>

template <typename T>
class TestRequiredSamples : public gr::Block<TestRequiredSamples<T>>
{
public:
    using Description = gr::Doc<R""()"">;

public:
    gr::PortIn<T> in;
    // This compiles fine:
    gr::PortOut<T> out;
    // This causes compile errors:
    // gr::PortOut<T, gr::RequiredSamples<1U, 1U, true>> out;

    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan,
                                 ::gr::OutputSpanLike auto& outSpan)
    {
        std::ignore = inSpan.consume(inSpan.size());
        outSpan.publish(1);
        outSpan[0] = T{ 0 };
        return gr::work::Status::OK;
    }

    GR_MAKE_REFLECTABLE(TestRequiredSamples, in, out);
};

int main()
{
    using namespace boost::ut;

    gr::Graph fg;
    fg.emplaceBlock<TestRequiredSamples<int>>();
    gr::scheduler::Simple sched;
    gr::packet_modem::init_scheduler(sched, std::move(fg));
    expect(sched.runAndWait().has_value());

    return 0;
}

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>
#include <gnuradio-4.0/packet-modem/scheduler_helpers.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <boost/ut.hpp>

#include <print>
class Source : public gr::Block<Source>
{
public:
    gr::PortOut<int> out;
    gr::MsgPortOut msg_out;

    size_t d_remaining = 1000000U;

    gr::work::Status processBulk(::gr::OutputSpanLike auto& outSpan)
    {
        std::println("Source::processBulk(outSpan.size() = {}), d_remaining = {}",
                     outSpan.size(),
                     d_remaining);
        gr::sendMessage<gr::message::Command::Invalid>(
            msg_out, "", "", gr::property_map{});
        const auto n = std::min(d_remaining, outSpan.size());
        outSpan.publish(n);
        std::println("Source published {}", n);
        d_remaining -= n;
        return d_remaining == 0 ? gr::work::Status::DONE : gr::work::Status::OK;
    }

    GR_MAKE_REFLECTABLE(Source, out, msg_out);
};

class ZeroSource : public gr::Block<ZeroSource>
{
public:
    gr::PortOut<int> out;

    gr::work::Status processBulk(::gr::OutputSpanLike auto& outSpan)
    {
        std::println("ZeroSource::processBulk(outSpan.size() = {})", outSpan.size());
        outSpan.publish(0);
        return gr::work::Status::DONE;
    }

    GR_MAKE_REFLECTABLE(ZeroSource, out);
};

class Sink : public gr::Block<Sink>
{
public:
    gr::PortIn<int> in;

    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan)
    {
        std::println("Sink::processBulk(inSpan.size() = {})", inSpan.size());
        std::ignore = inSpan.consume(inSpan.size());
        return gr::work::Status::OK;
    }

    GR_MAKE_REFLECTABLE(Sink, in);
};

class MessageSink : public gr::Block<MessageSink>
{
public:
    gr::MsgPortIn msg_in;
    gr::PortIn<int> fake_in; // needed because otherwise the block doesn't compile

    void processMessages(gr::MsgPortIn&,
                         std::span<const gr::Message> messages)
    {
        std::println("MessageSink::processMessages(messages.size() = {})",
                     messages.size());
    }

    void processOne(int) { return; }

    GR_MAKE_REFLECTABLE(MessageSink, fake_in, msg_in);
};

int main()
{
    using namespace boost::ut;

    gr::Graph fg;
    auto& source = fg.emplaceBlock<Source>();
    auto& zero_source = fg.emplaceBlock<ZeroSource>();
    auto& sink = fg.emplaceBlock<Sink>();
    auto& msg_sink = fg.emplaceBlock<MessageSink>();
    expect(eq(gr::ConnectionResult::SUCCESS, fg.connect<"out">(source).to<"in">(sink)));
    expect(eq(gr::ConnectionResult::SUCCESS,
              fg.connect<"out">(zero_source).to<"fake_in">(msg_sink)));
    expect(eq(gr::ConnectionResult::SUCCESS,
              fg.connect<"msg_out">(source).to<"msg_in">(msg_sink)));
    gr::scheduler::Simple sched;
    gr::packet_modem::init_scheduler(sched, std::move(fg));
    expect(sched.runAndWait().has_value());

    return 0;
}

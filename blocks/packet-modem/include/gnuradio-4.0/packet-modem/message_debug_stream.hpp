#ifndef _GR4_PACKET_MODEM_MESSAGE_DEBUG_STREAM
#define _GR4_PACKET_MODEM_MESSAGE_DEBUG_STREAM

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <algorithm>
#include <iterator>
#include <vector>

#include <print>
namespace gr::packet_modem {

class MessageDebugStream : public gr::Block<MessageDebugStream>
{
public:
    using Description = Doc<R""(
@brief Message Debug (Stream Input). Prints received messages.

Messages received in the `print` port are printed. This is like Message Debug
but with a stream input.

)"">;

public:
    gr::PortIn<gr::Message> print;

    void processOne(const gr::Message& message)
    {
        const auto& data = message.data;
        if (data.has_value()) {
            std::println("[MessageDebugStream] {}", data.value());
        } else {
            std::println("[MessageDebugStream] ERROR: {}", data.error());
        }
    }

    GR_MAKE_REFLECTABLE(MessageDebugStream, print);
};

} // namespace gr::packet_modem

#endif // _GR4_PACKET_MODEM_MESSAGE_DEBUG_STREAM

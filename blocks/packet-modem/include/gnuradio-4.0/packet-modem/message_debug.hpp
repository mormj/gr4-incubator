#ifndef _GR4_PACKET_MODEM_MESSAGE_DEBUG
#define _GR4_PACKET_MODEM_MESSAGE_DEBUG

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <algorithm>
#include <iterator>
#include <vector>

#include <print>
namespace gr::packet_modem {

class MessageDebug : public gr::Block<MessageDebug>
{
public:
    using Description = Doc<R""(
@brief Message Debug. Prints or stores received messages.

Messages received in the `print` port are printed. Messages received in the
`store` port are stored. The stored messages can be retrieved with the
`messages()` method.

)"">;

public:
    std::vector<gr::Message> _messages;

public:
    gr::MsgPortIn print;
    gr::MsgPortIn store;

    void processMessages(gr::MsgPortIn& port, std::span<const gr::Message> messages)
    {
        if (&port == &print) {
            for (const auto& message : messages) {
                const auto& data = message.data;
                if (data.has_value()) {
                    std::println("[MessageDebug] {}", data.value());
                } else {
                    std::println("[MessageDebug] ERROR: {}", data.error());
                }
            }
            return;
        }

        if (&port == &store) {
            std::ranges::copy(messages, std::back_inserter(_messages));
        }
    }

    // Dummy stream input to satisfy processOne/processBulk requirements.
    gr::PortIn<int> fake_in;

    void processOne(int) { return; }

    std::vector<gr::Message> messages() { return _messages; }

    GR_MAKE_REFLECTABLE(MessageDebug, fake_in, print, store);
};

} // namespace gr::packet_modem

#endif // _GR4_PACKET_MODEM_MESSAGE_DEBUG

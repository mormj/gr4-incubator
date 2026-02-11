#ifndef _GR4_PACKET_MODEM_PDU_TO_TAGGED_STREAM
#define _GR4_PACKET_MODEM_PDU_TO_TAGGED_STREAM

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>
#include <gnuradio-4.0/packet-modem/pdu.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <ranges>

#include <format>
#include <print>
namespace gr::packet_modem {

template <typename T>
class PduToTaggedStream : public gr::Block<PduToTaggedStream<T>>
{
public:
    using Description = Doc<R""(
@brief PDU to Tagged Stream. Converts a stream of PDUs into a stream of packets
delimited by packet length tags.

The input of this block is a stream of `Pdu` objects. It converts this into a
stream of items of type `T` by using packet-length tags to delimit the packets.
The tags in the `tag` vector of the `Pdu`s are attached to the corresponding
items in the output stream.

Tags in the input stream are discarded. If `packet_len_tag_key` is the empty
string, packet-length tags delimiting the packets are not inserted.

)"">;

public:
    ssize_t _index = 0;
    uint64_t _packet_count;

public:
    gr::PortIn<Pdu<T>> in;
    gr::PortOut<T, gr::Async> out;
    gr::PortOut<gr::Message, gr::Async, gr::Optional> count;
    std::string packet_len_tag_key = "packet_len";
    bool enable_count = false;


    void start()
    {
        _index = 0;
        _packet_count = 0;
    }

    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan,
                                 ::gr::OutputSpanLike auto& outSpan,
                                 ::gr::OutputSpanLike auto& countSpan)
    {
#ifdef TRACE
        std::println("{}::processBulk(inSpan.size() = {}, outSpan.size() = {}, "
                     "countSpan.size() = {}), _index = {}, _packet_count = {}",
                     this->name,
                     inSpan.size(),
                     outSpan.size(),
                     countSpan.size(),
                     _index,
                     _packet_count);
#endif
        if (inSpan.size() == 0) {
            std::ignore = inSpan.consume(0);
            outSpan.publish(0);
            return gr::work::Status::INSUFFICIENT_INPUT_ITEMS;
        }
        auto in_item = inSpan.begin();
        auto out_item = outSpan.begin();
        auto count_item = countSpan.begin();
        while (in_item != inSpan.end() && out_item != outSpan.end() &&
               (!enable_count || count_item != countSpan.end())) {
            if (_index == 0) {
                const uint64_t packet_len = in_item->data.size();
                if (packet_len == 0) {
                    this->emitErrorMessage(std::format("{}::processBulk", this->name),
                                           "received PDU of length zero");
                    this->requestStop();
                    return gr::work::Status::ERROR;
                }
                if (!packet_len_tag_key.empty()) {
                    const gr::property_map map = gr::packet_modem::make_props(
                        { { packet_len_tag_key, packet_len } });
                    const auto index = out_item - outSpan.begin();
#ifdef TRACE
                    std::println("{} publishTag({}, {})", this->name, map, index);
#endif
                    out.publishTag(map, index);
                }
                if (enable_count) {
                    ++_packet_count;
                    gr::Message msg;
                    msg.data = gr::packet_modem::make_props(
                        { { "packet_count", _packet_count } });
                    *count_item++ = std::move(msg);
                }
            }
            const auto n =
                std::min(std::ssize(in_item->data) - _index, outSpan.end() - out_item);
            std::ranges::copy(
                in_item->data | std::views::drop(_index) | std::views::take(n), out_item);
            for (const auto& tag : in_item->tags) {
                if (tag.index >= _index && tag.index - _index < n) {
                    const auto index = tag.index - _index + (out_item - outSpan.begin());
#ifdef TRACE
                    std::println("{} publishTag({}, {})", this->name, tag.map, index);
#endif
                    out.publishTag(tag.map, index);
                }
            }
            out_item += n;
            _index += n;
            if (_index == std::ssize(in_item->data)) {
                ++in_item;
                _index = 0;
            }
        }

        std::ignore = inSpan.consume(static_cast<size_t>(in_item - inSpan.begin()));
        outSpan.publish(static_cast<size_t>(out_item - outSpan.begin()));
        if (enable_count) {
            countSpan.publish(static_cast<size_t>(count_item - countSpan.begin()));
        }
#ifdef TRACE
        std::println("{} consumed = {}, published = {}, published countSpan = {}",
                     this->name,
                     in_item - inSpan.begin(),
                     out_item - outSpan.begin(),
                     count_item - countSpan.begin());
#endif

        return out_item == outSpan.begin() ? gr::work::Status::INSUFFICIENT_OUTPUT_ITEMS
                                           : gr::work::Status::OK;
    }
    GR_MAKE_REFLECTABLE(PduToTaggedStream, in, out, count, packet_len_tag_key, enable_count);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_PDU_TO_TAGGED_STREAM

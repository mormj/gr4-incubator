#ifndef _GR4_PACKET_MODEM_PACKET_INGRESS
#define _GR4_PACKET_MODEM_PACKET_INGRESS

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>
#include <gnuradio-4.0/packet-modem/packet_type.hpp>
#include <gnuradio-4.0/packet-modem/pdu.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <ranges>

#include <format>
#include <print>
namespace gr::packet_modem {

template <typename T = uint8_t>
class PacketIngress : public gr::Block<PacketIngress<T>>
{
public:
    using Description = Doc<R""(
@brief Packet Ingress. Accepts packets into a packet-based transmitter.

This block is the point at which packets enter into packet-based
transmitter. Packets are delimited at the input of this block by packet-length
tags, and presented back-to-back. For each valid packet, the block sends a
message to the header formatter indicating the length of the packet, so that the
header formatter can produce a header to be attached to the packet. Invalid
packets, which are those longer than the maximum packet length that fits in the
header, are consumed and discarded, and a warning is printed.

The block could be extended by the user to perform any other validation checks
or payload preparation that is necessary.

)"">;

public:
    size_t _remaining;
    bool _valid;

public:
    gr::PortIn<T> in;
    gr::PortOut<T> out;
    gr::PortOut<gr::Message, gr::Async> metadata;
    std::string packet_len_tag_key = "packet_len";


    void start() { _remaining = 0; }

    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan,
                                 ::gr::OutputSpanLike auto& outSpan,
                                 ::gr::OutputSpanLike auto& metadataSpan)
    {
#ifdef TRACE
        std::println("{}::processBulk(inSpan.size() = {}, outSpan.size() = {}, "
                     "metadataSpan.size() = {}), "
                     "_remaining = {}, _valid = {}",
                     this->name,
                     inSpan.size(),
                     outSpan.size(),
                     metadataSpan.size(),
                     _remaining,
                     _valid);
#endif
        assert(inSpan.size() > 0);
        assert(inSpan.size() == outSpan.size());
        assert(metadataSpan.size() > 0);
        auto meta = metadataSpan.begin();
        if (_remaining == 0) {
            // Fetch a new packet_len tag
            auto not_found_error = [this]() {
                this->emitErrorMessage(std::format("{}::processBulk", this->name),
                                       "expected packet-length tag not found");
                this->requestStop();
                return gr::work::Status::ERROR;
            };
            if (!this->inputTagsPresent()) {
                return not_found_error();
            }
            auto tag = this->mergedInputTag();
            const auto len_it = gr::packet_modem::find_prop(tag.map, packet_len_tag_key);
            if (len_it == tag.map.end()) {
                return not_found_error();
            }
            _remaining = gr::packet_modem::pmt_cast<uint64_t>(len_it->second);
            _valid = _remaining <= std::numeric_limits<uint16_t>::max();
            if (_valid) {
                // default to user data if packet type not indicated
                PacketType packet_type = PacketType::USER_DATA;
                if (const auto type_it = gr::packet_modem::find_prop(tag.map, "packet_type");
                    type_it != tag.map.end()) {
                    packet_type = magic_enum::enum_cast<PacketType>(
                                      gr::packet_modem::pmt_cast<std::string>(type_it->second),
                                      magic_enum::case_insensitive)
                                      .value();
                }
                gr::Message msg;
                msg.data = gr::packet_modem::make_props({
                    { "packet_length", _remaining },
                    { "packet_type",
                      gr::packet_modem::pmt_value(std::string(magic_enum::enum_name(packet_type))) },
                });
                *meta++ = std::move(msg);
#ifdef TRACE
                std::println("{} publishTag({}, 0)", this->name, tag.map);
#endif
                out.publishTag(tag.map, 0);
            } else {
                std::println(
                    "{} packet too long (length {}); dropping", this->name, _remaining);
            }
        } else if (_valid && this->inputTagsPresent()) {
#ifdef TRACE
            std::println("{} publishTag({}, 0)", this->name, this->mergedInputTag().map);
#endif
            out.publishTag(this->mergedInputTag().map, 0);
        }

        const auto to_consume = std::min(_remaining, inSpan.size());
        if (_valid) {
            std::ranges::copy_n(
                inSpan.begin(), static_cast<ssize_t>(to_consume), outSpan.begin());
            outSpan.publish(to_consume);
        } else {
            outSpan.publish(0);
        }
        metadataSpan.publish(static_cast<size_t>(meta - metadataSpan.begin()));
        if (!inSpan.consume(to_consume)) {
            throw gr::exception("consume failed");
        }
        _remaining -= to_consume;
        assert(to_consume > 0);

        return gr::work::Status::OK;
    }

    GR_MAKE_REFLECTABLE(PacketIngress, in, out, metadata, packet_len_tag_key);
};


template <typename T>
class PacketIngress<Pdu<T>> : public gr::Block<PacketIngress<Pdu<T>>>
{
public:
    using Description = PacketIngress<T>::Description;

public:
    size_t _remaining;
    bool _valid;

public:
    gr::PortIn<Pdu<T>> in;
    gr::PortOut<Pdu<T>> out;
    gr::PortOut<gr::Message> metadata;
    std::string packet_len_tag_key = "packet_len";

    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan,
                                 ::gr::OutputSpanLike auto& outSpan,
                                 ::gr::OutputSpanLike auto& metadataSpan)
    {
#ifdef TRACE
        std::println("{}::processBulk(inSpan.size() = {}, outSpan.size() = {}, "
                     "metadataSpan.size() = {})",
                     this->name,
                     inSpan.size(),
                     outSpan.size(),
                     metadataSpan.size());
#endif
        assert(inSpan.size() > 0);
        assert(inSpan.size() == outSpan.size());
        assert(inSpan.size() == metadataSpan.size());
        size_t consumed = 0;
        size_t produced = 0;
        while (consumed < inSpan.size()) {
            const uint64_t packet_length = inSpan[consumed].data.size();
#ifdef TRACE
            std::println("{} packet_length = {}", this->name, packet_length);
#endif
            if (packet_length == 0) {
                throw gr::exception("packet_length = 0");
            } else if (packet_length <= std::numeric_limits<uint16_t>::max()) {
                outSpan[produced] = inSpan[consumed];
                // default to user data if packet type not indicated
                PacketType packet_type = PacketType::USER_DATA;
                if (!outSpan[produced].tags.empty() &&
                    outSpan[produced].tags[0].index == 0) {
                    const auto& map = outSpan[produced].tags[0].map;
                    if (const auto type_it = gr::packet_modem::find_prop(map, "packet_type");
                        type_it != map.end()) {
                        packet_type = magic_enum::enum_cast<PacketType>(
                                          gr::packet_modem::pmt_cast<std::string>(type_it->second),
                                          magic_enum::case_insensitive)
                                          .value();
                    }
                }
                gr::Message msg;
                msg.data = gr::packet_modem::make_props(
                    { { "packet_length", packet_length },
                      { "packet_type",
                        gr::packet_modem::pmt_value(std::string(magic_enum::enum_name(packet_type))) } });
                metadataSpan[produced] = std::move(msg);
                ++produced;
            } else {
                std::println("{} packet too long (length {}); dropping",
                             this->name,
                             packet_length);
            }
            ++consumed;
        }
        if (!inSpan.consume(consumed)) {
            throw gr::exception("consume failed");
        }
        outSpan.publish(produced);
        metadataSpan.publish(produced);
#ifdef TRACE
        std::println("{} consumed = {}, produced = {}", this->name, consumed, produced);
#endif
        return gr::work::Status::OK;
    }
    GR_MAKE_REFLECTABLE(PacketIngress, in, out, metadata, packet_len_tag_key);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_PACKET_INGRESS

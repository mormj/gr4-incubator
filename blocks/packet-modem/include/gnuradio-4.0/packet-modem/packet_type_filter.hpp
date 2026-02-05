#ifndef _GR4_PACKET_MODEM_PACKET_TYPE_FILTER
#define _GR4_PACKET_MODEM_PACKET_TYPE_FILTER

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
class PacketTypeFilter : public gr::Block<PacketTypeFilter<T>>
{
public:
    using Description = Doc<R""(
@brief Packet Type Filter. Filters packets according to their packet type.

This block only lets through packets of a certain packet type, dropping
everything else.

)"">;

public:
    size_t _remaining;
    bool _valid;

public:
    gr::PortIn<T> in;
    gr::PortOut<T> out;
    std::string packet_len_tag_key = "packet_len";
    PacketType _packet_type = PacketType::USER_DATA;
    std::string packet_type{ magic_enum::enum_name(_packet_type) };


    void settingsChanged(const gr::property_map& /* old_settings */,
                         const gr::property_map& /* new_settings */)
    {
        _packet_type =
            magic_enum::enum_cast<PacketType>(packet_type, magic_enum::case_insensitive)
                .value();
    }

    void start() { _remaining = 0; }

    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan,
                                 ::gr::OutputSpanLike auto& outSpan)
    {
#ifdef TRACE
        std::println("{}::processBulk(inSpan.size() = {}, outSpan.size() = {}), "
                     "_remaining = {}, _valid = {}",
                     this->name,
                     inSpan.size(),
                     outSpan.size(),
                     _remaining,
                     _valid);
#endif
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
            const auto len_it = tag.map.find(packet_len_tag_key);
            if (len_it == tag.map.end()) {
                return not_found_error();
            }
            _remaining = gr::packet_modem::pmt_cast<uint64_t>(len_it->second);
            const auto type_it = tag.map.find("packet_type");
            if (type_it == tag.map.end()) {
                return not_found_error();
            }
            const auto packet_type_in =
                magic_enum::enum_cast<PacketType>(
                    gr::packet_modem::pmt_cast<std::string>(type_it->second),
                    magic_enum::case_insensitive)
                    .value();
            _valid = packet_type_in == _packet_type;
            if (_valid) {
#ifdef TRACE
                std::println("{} publishTag({}, 0)", this->name, tag.map);
#endif
                out.publishTag(tag.map, 0);
            }
        } else if (_valid && this->inputTagsPresent()) {
#ifdef TRACE
            std::println("{} publishTag({}, 0)", this->name, this->mergedInputTag().map);
#endif
            out.publishTag(this->mergedInputTag().map, 0);
        }

        auto to_consume = std::min(_remaining, inSpan.size());
        if (_valid) {
            to_consume = std::min(to_consume, outSpan.size());
            std::ranges::copy_n(
                inSpan.begin(), static_cast<ssize_t>(to_consume), outSpan.begin());
            outSpan.publish(to_consume);
        } else {
            outSpan.publish(0);
        }
        _remaining -= to_consume;
        if (!inSpan.consume(to_consume)) {
            throw gr::exception("consume failed");
        }

        return gr::work::Status::OK;
    }
    GR_MAKE_REFLECTABLE(PacketTypeFilter, in, out, packet_len_tag_key, packet_type);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_PACKET_TYPE_FILTER

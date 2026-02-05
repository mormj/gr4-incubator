#ifndef _GR4_PACKET_MODEM_MULTIPLY_PACKET_LEN_TAG
#define _GR4_PACKET_MODEM_MULTIPLY_PACKET_LEN_TAG

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>

#include <print>
namespace gr::packet_modem {

template <typename T>
class MultiplyPacketLenTag : public gr::Block<MultiplyPacketLenTag<T>>
{
public:
    using Description = Doc<R""(
@brief Multiply packet length tag.

This block multiplies packet length tags by a fixed given value.

)"">;

public:
    gr::PortIn<T> in;
    gr::PortOut<T> out;
    double mult = 1.0;
    std::string packet_len_tag_key = "packet_len";

    // this needs custom tag propagation because it overwrites tags

    [[nodiscard]] constexpr T processOne(T a) noexcept
    {
        if (this->inputTagsPresent()) {
            auto tag = this->mergedInputTag();
            if (const auto it = gr::packet_modem::find_prop(tag.map, packet_len_tag_key);
                it != tag.map.end()) {
                uint64_t packet_len = gr::packet_modem::pmt_cast<uint64_t>(it->second);
                gr::packet_modem::set_prop(
                    tag.map,
                    packet_len_tag_key,
                    gr::packet_modem::pmt_value(static_cast<uint64_t>(
                        std::round(mult * static_cast<double>(packet_len)))));
            }
#ifdef TRACE
            std::println("{} publishing tag: map = {}", this->name, tag.map);
#endif
            out.publishTag(tag.map, 0);
        }
        return a;
    }
    GR_MAKE_REFLECTABLE(MultiplyPacketLenTag, in, out, mult, packet_len_tag_key);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_MULTIPLY_PACKET_LEN_TAG

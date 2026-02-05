#ifndef _GR4_PACKET_MODEM_HEADER_PARSER
#define _GR4_PACKET_MODEM_HEADER_PARSER

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/packet-modem/packet_type.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>

#include <print>
namespace gr::packet_modem {

static constexpr size_t HEADER_PARSER_HEADER_LEN = 4U;

template <typename T = uint8_t>
class HeaderParser : public gr::Block<HeaderParser<T>,
                                      gr::Resampling<HEADER_PARSER_HEADER_LEN, 1U, true>>
{
public:
    using Description = Doc<R""(
@brief Header Parser. Parses a header into a metadata message.

This block receives headers packed as 8 bits per byte in the input port and
for each header it generates a metadata message with the parsed values from
the header in the metadata port.

See Header Formatter for details about the header format.

)"">;

private:
    static constexpr size_t HEADER_LEN = HEADER_PARSER_HEADER_LEN;

public:
    gr::PortIn<uint8_t> in;
    gr::PortOut<gr::Message> metadata;


public:
    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan,
                                 ::gr::OutputSpanLike auto& outSpan)
    {
#ifdef TRACE
        std::println("{}::processBulk(inSpan.size() = {}, outSpan.size() = {})",
                     this->name,
                     inSpan.size(),
                     outSpan.size());
#endif
        assert(inSpan.size() == outSpan.size() * HEADER_LEN);
        assert(inSpan.size() > 0);
        auto header = inSpan.begin();
        for (auto& meta : outSpan) {
#ifdef TRACE
            std::println("{} header = {:02x} {:02x} {:02x} {:02x}",
                         this->name,
                         header[0],
                         header[1],
                         header[2],
                         header[3]);
#endif
            bool valid = true;
            if (this->inputTagsPresent() &&
                gr::packet_modem::has_prop(this->mergedInputTag().map, "invalid_header")) {
#ifdef TRACE
                std::println("{} LDPC decoder error", this->name);
#endif
                // LDPC decoder error
                valid = false;
            }
            const uint64_t packet_length = (static_cast<uint64_t>(header[0]) << 8) |
                                           static_cast<uint64_t>(header[1]);
            if (packet_length == 0) {
                valid = false;
            }
            const uint8_t packet_type_field = header[2];
            PacketType packet_type;
            if (packet_type_field == 0x00) {
                packet_type = PacketType::USER_DATA;
            } else if (packet_type_field == 0x01) {
                packet_type = PacketType::IDLE;
            } else {
                valid = false;
            }
            gr::Message msg;
            if (valid) {
                msg.data = gr::packet_modem::make_props(
                    { { "packet_length", gr::packet_modem::pmt_value(packet_length) },
                      { "constellation", gr::packet_modem::pmt_value("QPSK") },
                      { "packet_type",
                        gr::packet_modem::pmt_value(std::string(magic_enum::enum_name(packet_type))) } });
            } else {
                msg.data = gr::packet_modem::make_props(
                    { { "invalid_header", gr::packet_modem::pmt_null() } });
            }
#ifdef TRACE
            std::println("{} sending message data {}", this->name, msg.data);
#endif
            meta = std::move(msg);

            header += HEADER_LEN;
        }

        return gr::work::Status::OK;
    }
    GR_MAKE_REFLECTABLE(HeaderParser, in, metadata);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_HEADER_PARSER

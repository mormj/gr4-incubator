#ifndef _GR4_PACKET_MODEM_STREAM_TO_PDU
#define _GR4_PACKET_MODEM_STREAM_TO_PDU

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/packet-modem/pdu.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <ranges>
#include <vector>

#include <print>
namespace gr::packet_modem {

template <typename T>
class StreamToPdu : public gr::Block<StreamToPdu<T>, gr::Resampling<>>
{
public:
    using Description = Doc<R""(
@brief Tagged Stream to PDU. Converts a stream of fixed size packets into a stream of PDUs.

The input of this block is a stream of items of type `T` forming packets of a
fixed length specified as a parameter of this block. The block converts the
packets in the input into a stream of `Pdu` objects, where each packet is stored
as the `data` vector in the `Pdu`. Tags are ignored by this block.

)"">;

public:
    gr::PortIn<T> in;
    gr::PortOut<Pdu<T>> out;
    size_t packet_length = 1;


    void settingsChanged(const gr::property_map& /* old_settings */,
                         const gr::property_map& /* new_settings */)
    {
        // set resampling for the scheduler
        this->input_chunk_size = packet_length;
        this->output_chunk_size = 1;
    }

    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan,
                                 ::gr::OutputSpanLike auto& outSpan)
    {
#ifdef TRACE
        std::println("{}::processBulk(inSpan.size() = {}, outSpan.size() = "
                     "{})",
                     this->name,
                     inSpan.size(),
                     outSpan.size());
#endif
        assert(inSpan.size() == outSpan.size() * packet_length);
        auto in_item = inSpan.begin();
        for (auto& out_item : outSpan) {
            out_item.data.clear();
            out_item.tags.clear();
            out_item.data.insert(out_item.data.end(),
                                 in_item,
                                 in_item + static_cast<ssize_t>(packet_length));
            in_item += static_cast<ssize_t>(packet_length);
        }

        return gr::work::Status::OK;
    }
    GR_MAKE_REFLECTABLE(StreamToPdu, in, out, packet_length);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_STREAM_TO_PDU

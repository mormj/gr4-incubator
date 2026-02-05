#ifndef _GR4_PACKET_MODEM_PDU_TO_VECTOR
#define _GR4_PACKET_MODEM_PDU_TO_VECTOR

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <gnuradio-4.0/packet-modem/pdu.hpp>

namespace gr::packet_modem {

template <typename T>
class PduToVector : public gr::Block<PduToVector<T>>
{
public:
    using Description = Doc<R""(
@brief PDU to vector. Drops tags and outputs the packet data as a std::vector.

)"">;

    gr::PortIn<Pdu<T>> in;
    gr::PortOut<std::vector<T>> out;

    [[nodiscard]] std::vector<T> processOne(const Pdu<T>& pdu) { return pdu.data; }

    GR_MAKE_REFLECTABLE(PduToVector, in, out);
};

} // namespace gr::packet_modem

#endif // _GR4_PACKET_MODEM_PDU_TO_VECTOR

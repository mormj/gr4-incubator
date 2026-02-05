#ifndef _GR4_PACKET_MODEM_TAG_GATE
#define _GR4_PACKET_MODEM_TAG_GATE

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>

namespace gr::packet_modem {

template <typename T>
class TagGate : public gr::Block<TagGate<T>>
{
public:
    using Description = Doc<R""(
@brief Tag Gate. Blocks tag propagation through the block.

)"">;


public:
    gr::PortIn<T> in;
    gr::PortOut<T> out;

    [[nodiscard]] constexpr T processOne(T a) noexcept { return a; }
    GR_MAKE_REFLECTABLE(TagGate, in, out);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_TAG_GATE

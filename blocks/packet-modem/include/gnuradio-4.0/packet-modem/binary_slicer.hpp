#ifndef _GR4_PACKET_MODEM_BINARY_SLICER
#define _GR4_PACKET_MODEM_BINARY_SLICER

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>

namespace gr::packet_modem {

template <bool invert = false, typename TIn = float, typename TOut = uint8_t>
class BinarySlicer : public gr::Block<BinarySlicer<invert, TIn, TOut>>
{
public:
    using Description = Doc<R""(
@brief Binary slicer. Transforms soft symbols into hard symbols.

This block transforms soft symbols into hard symbols by comparing with a
threshold at zero. The `invert` template parameter controls whether a positive
soft symbol is decoded as the bit 1 (`invert = false`) or the bit 0 (`invert =
true`).

)"">;

public:
    gr::PortIn<TIn> in;
    gr::PortOut<TOut> out;

    [[nodiscard]] constexpr TOut processOne(TIn a) noexcept
    {
        if constexpr (invert) {
            return a < 0;
        } else {
            return a > 0;
        }
    }
    GR_MAKE_REFLECTABLE(BinarySlicer, in, out);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_BINARY_SLICER

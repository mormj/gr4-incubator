#pragma once

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>



namespace gr::incubator::basic {

GR_REGISTER_BLOCK("gr::incubator::basic::Copy", gr::incubator::basic::Copy, ([T]), [ uint8_t, int16_t, int32_t ])

template<typename T>
struct Copy : Block<Copy<T>> {

    using Description = Doc<
        "Identity 1:1 passthrough block. Forwards every input sample to the output unchanged. "
        "Useful as: a fan-out adapter connecting one source to multiple downstream consumers; "
        "a tag injection point to attach tags without modifying the data path; "
        "a chain placeholder while the real processing block is being written; "
        "or a benchmark baseline to measure graph scheduling overhead with zero compute.">;

    PortIn<T> in;
    PortOut<T> out;

    GR_MAKE_REFLECTABLE(Copy, in, out);

    [[nodiscard]] constexpr T processOne(T input) const noexcept { return input; }
};

} // namespace gr::incubator::basic


#ifndef _GR4_PACKET_MODEM_CONSTELLATION_LLR_DECODER
#define _GR4_PACKET_MODEM_CONSTELLATION_LLR_DECODER

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/packet-modem/constellation.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <magic_enum.hpp>
#include <complex>

#include <format>
#include <print>
namespace gr::packet_modem {

template <typename T = float>
class ConstellationLLRDecoder
    : public gr::Block<ConstellationLLRDecoder<T>, gr::Resampling<>>
{
public:
    using Description = Doc<R""(
@brief Constellation LLR decoder.

This block converts symbols from a constellation into LLRs (log-likelihood
ratios) for each bit. The constellation can be updated on the fly by using
`"constellation"` tags.

This block uses the convention that in BPSK and QPSK a negative amplitude
encodes the bit 1. It uses the LLR convention that a positive LLR means that the
bit 0 is more likely.

)"">;

public:
    bool _in_syncword = false;
    size_t _position = 0;

private:
    static constexpr char syncword_amplitude_key[] = "syncword_amplitude";

public:
    T _scale = T{ 2 };

public:
    gr::PortIn<std::complex<T>> in;
    gr::PortOut<T> out;
    // standard deviation of the noise in the real part (and also imaginary
    // part) of the complex input AGWN
    T noise_sigma = T{ 1 };
    Constellation _constellation = Constellation::BPSK;
    std::string constellation{ magic_enum::enum_name(_constellation) };

    // use custom tag propagation policy because the runtime isn't smart enough
    // to propagate tags correctly with the `this->numerator` changes done by
    // this block

    void settingsChanged(const gr::property_map& /* old_settings */,
                         const gr::property_map& /* new_settings */)
    {
#ifdef TRACE
        std::println(
            "{}::settingsChanged(), constellation = {}", this->name, constellation);
#endif
        // set resampling for the scheduler
        _constellation = magic_enum::enum_cast<Constellation>(
                             constellation, magic_enum::case_insensitive)
                             .value();
        this->input_chunk_size = 1;
        switch (_constellation) {
        case Constellation::BPSK:
            this->output_chunk_size = 1;
            break;
        case Constellation::QPSK:
            this->output_chunk_size = 2;
            break;
        default:
            throw gr::exception(
                std::format("constellation {} not supported", constellation));
        }
        _scale = T{ 2 } / (noise_sigma * noise_sigma);
    }

    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan,
                                 ::gr::OutputSpanLike auto& outSpan)
    {
#ifdef TRACE
        std::println("{}::processBulk(inSpan.size() = {}, outSpan.size() = {}), "
                     "constellation = {}, output_chunk_size = {}",
                     this->name,
                     inSpan.size(),
                     outSpan.size(),
                     constellation,
                     this->output_chunk_size);
#endif
        if (this->inputTagsPresent()) {
            const auto tag = this->mergedInputTag();
#ifdef TRACE
            std::println("{} tag = {}, index = {}", this->name, tag.map, tag.index);
#endif
            out.publishTag(tag.map, 0);
        }

        const auto n = std::min(inSpan.size(), outSpan.size() / this->output_chunk_size);

        auto out_item = outSpan.begin();
        auto input = inSpan | std::views::take(n);
        switch (_constellation) {
        case Constellation::BPSK:
            for (auto& in_item : input) {
                *out_item++ = _scale * in_item.real();
            }
            break;
        case Constellation::QPSK:
            for (auto& in_item : input) {
                *out_item++ = _scale * in_item.real();
                *out_item++ = _scale * in_item.imag();
            }
            break;
        default:
            // should not be reached
            abort();
        }

        if (!inSpan.consume(n)) {
            throw gr::exception("consume failed");
        }
        outSpan.publish(n * this->output_chunk_size);
#ifdef TRACE
        std::println("{} consumed = {}, produced = {}",
                     this->name,
                     n,
                     n * this->output_chunk_size);
#endif

        return gr::work::Status::OK;
    }
    GR_MAKE_REFLECTABLE(ConstellationLLRDecoder, in, out, noise_sigma, constellation);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_CONSTELLATION_LLR_DECODER

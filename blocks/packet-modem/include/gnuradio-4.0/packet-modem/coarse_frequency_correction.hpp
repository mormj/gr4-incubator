#include <print>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>
/*
 * This file contains code adapted from GNU Radio 3.10, which is licensed as
 * follows:
 *
 * Copyright 2003,2008,2013,2014 Free Software Foundation, Inc.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef _GR4_PACKET_MODEM_COARSE_FREQUENCY_CORRECTION
#define _GR4_PACKET_MODEM_COARSE_FREQUENCY_CORRECTION

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <complex>

namespace gr::packet_modem {

template <typename T = float>
class CoarseFrequencyCorrection : public gr::Block<CoarseFrequencyCorrection<T>>
{
public:
    using Description = Doc<R""(
@brief Coarse Frequency Correction. Corrects frequency according to the syncword
frequency tag.

This block uses "syncword_freq" tags to correct the frequency of the
signal. Each time that such a tag is received, the phase of an internal rotator
is set to zero and its frequency is set to the opposite of the
"syncword_freq". The rotator is used to rotate the signal and correct a constant
frequency offset that is updated with each syncword detection.

Optionally, a delay can be used for the application of tags. In this case, the
phase is not reset to zero, but rather to the new frequency times the delay.

)"">;

public:
    std::complex<T> _exp = { T{ 1 }, T{ 0 } };
    std::complex<T> _exp_incr = { T{ 1 }, T{ 0 } };
    unsigned _counter = 0;
    size_t delay = 0;
    float _next_freq = 0.0f;
    ssize_t _next_freq_delay = 0;

private:
    static constexpr char syncword_freq_key[] = "syncword_freq";

    void set_freq(float freq)
    {
#ifdef TRACE
        std::println("{}::set_freq({})", this->name, freq);
#endif
        _exp = { static_cast<T>(std::cos(freq * static_cast<float>(delay))),
                 static_cast<T>(-std::sin(freq * static_cast<float>(delay))) };
        _exp_incr = { static_cast<T>(std::cos(freq)), static_cast<T>(-std::sin(freq)) };
        _counter = 0;
    }

public:
    gr::PortIn<std::complex<T>> in;
    gr::PortOut<std::complex<T>> out;

    // processBulk() used instead of processOne() for the same reason as in
    // AdditiveScrambler
    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan,
                                 ::gr::OutputSpanLike auto& outSpan)
    {
#ifdef TRACE
        std::println("{}::processBulk(inSpan.size() = {}, outSpan.size() = {})",
                     this->name,
                     inSpan.size(),
                     outSpan.size());
#endif
        if (this->inputTagsPresent()) {
            const auto tag = this->mergedInputTag();
            if (const auto it = gr::packet_modem::find_prop(tag.map, syncword_freq_key);
                it != tag.map.end()) {
                _next_freq = gr::packet_modem::pmt_cast<float>(it->second);
                _next_freq_delay = static_cast<ssize_t>(delay);
            }
        }
        for (size_t j = 0; j < inSpan.size(); ++j) {
            if (_next_freq_delay == 0) {
                set_freq(_next_freq);
            }
            outSpan[j] = inSpan[j] * _exp;
            _exp *= _exp_incr;
            if ((++_counter % 512) == 0) {
                // normalize to unit amplitude
                _exp /= std::abs(_exp);
            }
            if (_next_freq_delay >= 0) {
                --_next_freq_delay;
            }
        }
        return gr::work::Status::OK;
    }
    GR_MAKE_REFLECTABLE(CoarseFrequencyCorrection, in, out, delay);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_COARSE_FREQUENCY_CORRECTION

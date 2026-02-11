#include <format>
#include <gnuradio-4.0/packet-modem/pmt_helpers.hpp>
#include <print>
/*
 * This file contains code adapted from GNU Radio 3.10, which is licensed as
 * follows:
 *
 * Copyright 2022 Daniel Estevez <daniel@destevez.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef _GR4_PACKET_MODEM_CRC_CHECK
#define _GR4_PACKET_MODEM_CRC_CHECK

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/packet-modem/crc.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <cstdint>
#include <ranges>

namespace gr::packet_modem {

template <typename CrcType = uint64_t>
class CrcCheck : public gr::Block<CrcCheck<CrcType>>
{
public:
    using Description = Doc<R""(
@brief CRC Check. Drops packets with wrong CRC.

The input of this block is a stream of `uint8_t` items forming packets delimited
by a packet-length tag that is present in the first item of the packet and whose
value indicates the length of the packet. The output has the same format.

For each packet, the block calculates its CRC and compares it to the CRC present
at the end of the packet. If the CRC matches, the packet is copied to the
output. If the CRC does not match or if the packet is too short to contain a
CRC, the packet is dropped.

The block can optionally work with CRCs with swapped byte-endianness by using
the `swap_endinanness` parameter, and skip a fixed number of "header bytes" in
the CRC calculation with the `skip_header_bytes` parameter. Additionally, the
CRC can be discarded in the output packets with the `discard_crc` parameter.

TODO: This block does not handle tags that appear mid-packet. This is because
the scheduler calls `processBulk()` once for every tag, but this block requires
the whole packet to be present in the input at once.

)"">;

public:
    unsigned _crc_num_bytes;
    // std::optional because it is constructed in settingsChanged()
    std::optional<Crc<CrcType>> _crc;
    uint64_t _packet_len;
    gr::Tag _tag;

private:
    void _set_crc()
    {
        _crc = Crc(
            num_bits, poly, initial_value, final_xor, input_reflected, result_reflected);
        _crc_num_bytes = num_bits / 8;
    }

public:
    gr::PortIn<uint8_t> in;
    gr::PortOut<uint8_t> out;
    // These defaults correspond to CRC-32, which is also the default in the GNU
    // Radio 3.10 block
    unsigned num_bits = 32;
    CrcType poly = static_cast<CrcType>(0x4C11DB7);
    CrcType initial_value = static_cast<CrcType>(0xFFFFFFFF);
    CrcType final_xor = static_cast<CrcType>(0xFFFFFFFF);
    bool input_reflected = true;
    bool result_reflected = true;
    bool swap_endianness = false;
    bool discard_crc = false;
    uint64_t skip_header_bytes = 0;
    std::string packet_len_tag_key = "packet_len";


    void settingsChanged(const gr::property_map& /* old_settings */,
                         const gr::property_map& /* new_settings */)
    {
        if (num_bits % 8 != 0) {
            throw gr::exception("CRC number of bits must be a multiple of 8");
        }
        _set_crc();
    }

    void start()
    {
        _packet_len = 0;
        _set_crc();
    }

    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan,
                                 ::gr::OutputSpanLike auto& outSpan)
    {
#ifdef TRACE
        std::println("{}::processBulk(inSpan.size() = {}, outSpan.size = {}), "
                     "inputTagsPresent() = {}, _tag.map = {}",
                     this->name,
                     inSpan.size(),
                     outSpan.size(),
                     this->inputTagsPresent(),
                     _tag.map);
#endif
        assert(inSpan.size() > 0);
        if (_packet_len == 0) {
            // Tags can only be fetched in one processBulk() call. They
            // disappear in the next call, even if the input is not consumed. We
            // store the value in case we need it.

            // Fetch the packet length tag to determine the length of the packet.
            auto not_found_error = [this]() {
                this->emitErrorMessage(std::format("{}::processBulk", this->name),
                                       "expected packet-length tag not found");
                this->requestStop();
                return gr::work::Status::ERROR;
            };
            if (!this->inputTagsPresent()) {
                return not_found_error();
            }
            _tag = this->mergedInputTag();
            const auto it = _tag.map.find(packet_len_tag_key);
            if (it == _tag.map.end()) {
                return not_found_error();
            }
            _packet_len = gr::packet_modem::pmt_cast<uint64_t>(it->second);
            if (_packet_len == 0) {
                this->emitErrorMessage(std::format("{}::processBulk", this->name),
                                       "received packet-length equal to zero");
                this->requestStop();
                return gr::work::Status::ERROR;
            }
        }

        if (inSpan.size() < _packet_len) {
            // we need the whole packet to be present in the input at once to
            // decide its fate (it can be dropped or passed to the output)
#ifdef TRACE
            std::println("{}::processBulk() -> INSUFFICIENT_INPUT_ITEMS", this->name);
#endif
            if (!inSpan.consume(0)) {
                throw gr::exception("consume failed");
            }
            outSpan.publish(0);
            return gr::work::Status::INSUFFICIENT_INPUT_ITEMS;
        }
        if (_packet_len <= _crc_num_bytes) {
            // the packet is too short; drop it
            std::println(
                "{} packet is too short (length {}); dropping", this->name, _packet_len);
            if (!inSpan.consume(_packet_len)) {
                throw gr::exception("consume failed");
            }
            outSpan.publish(0);
            _packet_len = 0;
            return gr::work::Status::OK;
        }

        const auto payload_size = _packet_len - _crc_num_bytes;
        const auto crc_computed =
            _crc->compute(inSpan | std::views::take(static_cast<ssize_t>(payload_size)) |
                          std::views::drop(skip_header_bytes));

        // Read CRC from packet
        CrcType crc_packet = CrcType{ 0 };
        if (swap_endianness) {
            for (size_t i = _packet_len - 1; i >= payload_size; --i) {
                crc_packet <<= 8;
                crc_packet |= inSpan[i];
            }
        } else {
            for (size_t i = payload_size; i < _packet_len; ++i) {
                crc_packet <<= 8;
                crc_packet |= inSpan[i];
            }
        }

        const bool crc_ok = crc_packet == crc_computed;
#ifdef TRACE
        std::println("{}::processBulk() crc_ok = {}", this->name, crc_ok);
#endif

        if (crc_ok) {
            const size_t output_size = discard_crc ? payload_size : _packet_len;
            if (outSpan.size() < output_size) {
#ifdef TRACE
                std::println("{}::processBulk() -> INSUFFICIENT_OUTPUT_ITEMS",
                             this->name);
#endif
                outSpan.publish(0);
                if (!inSpan.consume(0)) {
                    throw gr::exception("consume failed");
                }
                return gr::work::Status::INSUFFICIENT_OUTPUT_ITEMS;
            }
            // modify packet_len tag
            gr::packet_modem::set_prop(_tag.map, packet_len_tag_key,
                                       output_size);
            out.publishTag(_tag.map, 0);
            std::ranges::copy_n(
                inSpan.begin(), static_cast<ssize_t>(output_size), outSpan.begin());
            outSpan.publish(output_size);
        } else {
            outSpan.publish(0);
        }

        if (!inSpan.consume(_packet_len)) {
            throw gr::exception("consume failed");
        }
        _packet_len = 0;

#ifdef TRACE
        std::println("{}::processBulk() -> OK", this->name);
#endif

        return gr::work::Status::OK;
    }
    GR_MAKE_REFLECTABLE(CrcCheck, in, out, num_bits, poly, initial_value, final_xor, input_reflected, result_reflected, swap_endianness, discard_crc, skip_header_bytes, packet_len_tag_key);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_CRC_CHECK

#ifndef _GR4_PACKET_MODEM_VECTOR_SOURCE
#define _GR4_PACKET_MODEM_VECTOR_SOURCE

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <ranges>
#include <stdexcept>
#include <vector>

#include <format>
#include <print>
namespace gr::packet_modem {

template <typename T>
struct VectorSource : public gr::Block<VectorSource<T>>
{
public:
    using Description = Doc<R""(
@brief Vector Source. Produces output from a given vector, either once or repeatedly.

This block is given a vector `data`. The output it produces is the samples in
`data`, either once or repeated indefinitely, according to the `repeat`
parameter.

The block can also generate tags, which are given as a vector in the `tags`
parameter. The indices of these tags should all be in the range
`[0, data.size())`. If the block is set to repeat, tags will also be repeated in
the same relative positions of the data.

)"">;


    size_t _position = 0;

    void check_vector()
    {
        if (data.empty()) {
            throw gr::exception("data is empty");
        }
        for (const auto& tag : tags) {
            if (tag.index < 0 || static_cast<size_t>(tag.index) >= data.size()) {
                throw gr::exception(
                    std::format("tag {} has invalid index {}", tag.map, tag.index));
            }
        }
    }


    gr::PortOut<T> out;
    std::vector<T> data;
    bool repeat = false;
    // this cannot be updated through settings because gr::Tag cannot be
    // converted to pmtv
    std::vector<gr::Tag> tags;

    // void settingsChanged(const gr::property_map& /* old_settings */,
    //                      const gr::property_map& /* new_settings */)
    // {
    // }

    void start()
    {
        check_vector();
        _position = 0;
    }

    gr::work::Status processBulk(::gr::OutputSpanLike auto& outSpan)
    {
#ifdef TRACE
        std::println("{}::processBulk(outSpan.size() = {})", this->name, outSpan.size());
#endif
//         if (outSpan.size() == 0) {
//             outSpan.publish(0);
//             return gr::work::Status::INSUFFICIENT_OUTPUT_ITEMS;
//         }

//         const size_t data_size = data.size();
//         size_t published = 0;

//         auto emit_tags = [&](size_t offset_in_data, size_t count, size_t out_base) {
//             if (count == 0) {
//                 return;
//             }
//             for (const auto& tag : tags) {
//                 if (tag.index < 0) {
//                     continue;
//                 }
//                 const auto tag_index = static_cast<size_t>(tag.index);
//                 if (tag_index >= offset_in_data &&
//                     tag_index < offset_in_data + count) {
//                     out.publishTag(tag.map,
//                                    static_cast<ssize_t>(
//                                        out_base + (tag_index - offset_in_data)));
//                 }
//             }
//         };

//         auto write_segment = [&](size_t offset_in_data, size_t count) {
//             std::copy_n(data.begin() + static_cast<ssize_t>(offset_in_data),
//                         static_cast<ssize_t>(count),
//                         outSpan.begin() + static_cast<ssize_t>(published));
//             emit_tags(offset_in_data, count, published);
//             published += count;
//         };

//         // First fill remaining items from the current position.
//         if (_position < data_size) {
//             const size_t n =
//                 std::min(data_size - _position, outSpan.size() - published);
//             write_segment(_position, n);
//             _position += n;
//         }

//         if (!repeat) {
//             outSpan.publish(published);
// #ifdef TRACE
//             if (_position == data_size) {
//                 std::println("{}::processBulk returning DONE", this->name);
//             } else {
//                 std::println("{}::processBulk returning OK", this->name);
//             }
// #endif
//             return _position == data_size ? gr::work::Status::DONE
//                                           : gr::work::Status::OK;
//         }

//         if (_position == data_size) {
//             _position = 0;
//         }

//         // Fill the rest of the output with full loops, then a partial loop.
//         while (published < outSpan.size()) {
//             const size_t n = std::min(data_size, outSpan.size() - published);
//             write_segment(0, n);
//             if (n < data_size) {
//                 _position = n;
//                 break;
//             }
//             _position = 0;
//         }

//         outSpan.publish(published);
// #ifdef TRACE
//         std::println("{}::processBulk returning OK", this->name);
// #endif
        outSpan.publish(outSpan.size());
        return gr::work::Status::OK;
    }
    GR_MAKE_REFLECTABLE(VectorSource, out, /*data, */ repeat);
};

} // namespace gr::packet_modem

// currently `data` is disabled from reflection because VectorSource is often
// used with Pdu<T>, which is not convertible to PMT because gr::Tag is not
// convertible to PMT


#endif // _GR4_PACKET_MODEM_VECTOR_SOURCE

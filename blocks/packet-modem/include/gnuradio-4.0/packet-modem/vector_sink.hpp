#ifndef _GR4_PACKET_MODEM_VECTOR_SINK
#define _GR4_PACKET_MODEM_VECTOR_SINK

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <vector>

#include <print>
namespace gr::packet_modem {

template <typename T>
class VectorSink : public gr::Block<VectorSink<T>>
{
public:
    using Description = Doc<R""(
@brief Vector Sink. Writes input items and tags into vectors.

This block writes all its input items into a `std::vector`, and all its input
tags into another `std::vector`. These vectors can be retrieved when the
flowgraph has stopped.

The `reserve_items` parameter indicates the initial number of items to reserve
in the item vector. The vector capacity will grow as needed if this number of
items is exceeded. The tag vector is constructed for an initial capacity of
zero, and grows as necessary.

)"">;

public:
    std::vector<T> _vector;
    std::vector<gr::Tag> _tags;

public:
    gr::PortIn<T> in;
    size_t reserve_items = 1024;

    void start() { _vector.reserve(reserve_items); }

    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan)
    {
#ifdef TRACE
        std::println("{}::processBulk(inSpan.size() = {})", this->name, inSpan.size());
#endif
        if (this->inputTagsPresent()) {
#ifdef TRACE
            std::println("{} received tag ({}, {}) at index = {}",
                         this->name,
                         this->mergedInputTag().index,
                         this->mergedInputTag().map,
                         _vector.size());
#endif
            _tags.emplace_back(_vector.size(), this->mergedInputTag().map);
        }

#ifdef __cpp_lib_containers_ranges
        _vector.append_range(inSpan);
#else
        // gcc-14 doesn't support append_range()
        _vector.insert(_vector.end(), inSpan.begin(), inSpan.end());
#endif

        if (!inSpan.consume(inSpan.size())) {
            throw gr::exception("consume failed");
        }
        return gr::work::Status::OK;
    }

    // Returns a copy of the vector stored in the block
    std::vector<T> data() const noexcept { return _vector; }

    // Returns a copy of the tags stored in the block
    std::vector<gr::Tag> tags() const noexcept { return _tags; }
    GR_MAKE_REFLECTABLE(VectorSink, in, reserve_items);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_VECTOR_SINK

#ifndef _GR4_PACKET_MODEM_PMT_HELPERS
#define _GR4_PACKET_MODEM_PMT_HELPERS

#include <gnuradio-4.0/Value.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/Graph.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace gr::packet_modem {

inline gr::pmt::Value pmt_null() { return gr::pmt::Value{}; }

template <typename T>
inline gr::pmt::Value pmt_value(T&& v) {
    return gr::pmt::Value(std::forward<T>(v));
}

template <typename T>
inline gr::pmt::Value pmt_value(const std::vector<T>& v) {
    return gr::pmt::Value(gr::Tensor<T>(gr::data_from, v));
}

template <typename T>
inline gr::pmt::Value pmt_value(std::vector<T>& v) {
    return gr::pmt::Value(gr::Tensor<T>(gr::data_from, v));
}

template <typename T>
inline gr::pmt::Value pmt_value(std::vector<T>&& v) {
    return gr::pmt::Value(gr::Tensor<T>(gr::data_from, v));
}

template <typename T>
inline T pmt_cast(const gr::pmt::Value& v) {
    if constexpr (std::is_same_v<T, std::string>) {
        return v.value_or(std::string{});
    } else if constexpr (std::is_same_v<T, std::string_view>) {
        return v.value_or(std::string_view{});
    } else {
        return v.value_or<T>(T{});
    }
}

inline gr::property_map make_props(std::initializer_list<std::pair<std::string_view, gr::pmt::Value>> init) {
    gr::property_map out;
    out.reserve(init.size());
    for (const auto& [key, value] : init) {
        out.emplace(std::pmr::string(key, std::pmr::get_default_resource()), value);
    }
    return out;
}

inline void set_prop(gr::property_map& map, std::string_view key, gr::pmt::Value value) {
    map[std::pmr::string(key, std::pmr::get_default_resource())] = std::move(value);
}

inline auto find_prop(gr::property_map& map, std::string_view key) {
    return map.find(std::pmr::string(key, std::pmr::get_default_resource()));
}

inline auto find_prop(const gr::property_map& map, std::string_view key) {
    return map.find(std::pmr::string(key, std::pmr::get_default_resource()));
}

inline bool has_prop(const gr::property_map& map, std::string_view key) {
    return find_prop(map, key) != map.end();
}

inline void erase_prop(gr::property_map& map, std::string_view key) {
    if (auto it = find_prop(map, key); it != map.end()) {
        map.erase(it);
    }
}

} // namespace gr::packet_modem

#endif // _GR4_PACKET_MODEM_PMT_HELPERS

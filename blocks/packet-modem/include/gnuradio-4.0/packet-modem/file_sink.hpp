#ifndef _GR4_PACKET_MODEM_FILE_SINK
#define _GR4_PACKET_MODEM_FILE_SINK

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <cerrno>
#include <cstdio>
#include <cstring>

#include <format>
#include <print>
namespace gr::packet_modem {

template <typename T>
class FileSink : public gr::Block<FileSink<T>>
{
public:
    using Description = Doc<R""(
@brief File Sink. Writes input items to a file.

This block writes all its input items into a binary file. The file can be
overwritten or appended, depending on the `append` parameter of the block constructor.

)"">;

public:
    FILE* _file = nullptr;

public:
    gr::PortIn<T> in;
    std::string filename;
    bool append = false;

    void start()
    {
        _file = std::fopen(filename.c_str(), append ? "ab" : "wb");
        if (_file == nullptr) {
            throw gr::exception(
                std::format("error opening file: {}", std::strerror(errno)));
        }
    }

    void stop()
    {
        if (_file != nullptr) {
            std::fclose(_file);
            _file = nullptr;
        }
    }

    gr::work::Status processBulk(::gr::InputSpanLike auto& inSpan)
    {
#ifdef TRACE
        std::println("{}::processBulk(inSpan.size() = {})", this->name, inSpan.size());
#endif
        const size_t n = inSpan.size();
        const size_t ret = fwrite(inSpan.data(), sizeof(T), n, _file);
        if (ret != n) {
            int errno_save = errno;
            std::println("{} fwrite failed: n = {}, ret = {}, errno = {}",
                         this->name,
                         n,
                         ret,
                         errno_save);
            this->emitErrorMessage(
                std::format("{}::processBulk", this->name),
                std::format("error writing to file: {}", std::strerror(errno_save)));
            this->requestStop();
            return gr::work::Status::ERROR;
        }
        return gr::work::Status::OK;
    }
    GR_MAKE_REFLECTABLE(FileSink, in, filename, append);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_FILE_SINK

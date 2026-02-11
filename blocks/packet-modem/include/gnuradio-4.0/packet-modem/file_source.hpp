#ifndef _GR4_PACKET_MODEM_FILE_SOURCE
#define _GR4_PACKET_MODEM_FILE_SOURCE

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/meta/reflection.hpp>
#include <cerrno>
#include <cstdio>
#include <cstring>

#include <format>
#include <print>
namespace gr::packet_modem {

template <typename T>
class FileSource : public gr::Block<FileSource<T>>
{
public:
    using Description = Doc<R""(
@brief File Source. Reads output items from a file.

This block reads data from a binary file and outputs the file contents as items.

)"">;

public:
    FILE* _file = nullptr;

public:
    gr::PortOut<T> out;
    std::string filename;

    void start()
    {
        _file = std::fopen(filename.c_str(), "rb");
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

    gr::work::Status processBulk(::gr::OutputSpanLike auto& outSpan)
    {
#ifdef TRACE
        std::println("{}::processBulk(outSpan.size() = {})", this->name, outSpan.size());
#endif
        const size_t n = outSpan.size();
        const size_t ret = fread(outSpan.data(), sizeof(T), n, _file);
        if (ret != n) {
            if (feof(_file)) {
                outSpan.publish(ret);
                return gr::work::Status::DONE;
            }

            int errno_save = errno;
            std::println("{} fread failed: n = {}, ret = {}, errno = {}",
                         this->name,
                         n,
                         ret,
                         errno_save);
            this->emitErrorMessage(
                std::format("{}::processBulk", this->name),
                std::format("error reading from file: {}", std::strerror(errno_save)));
            this->requestStop();
            return gr::work::Status::ERROR;
        }
        outSpan.publish(n);
        return gr::work::Status::OK;
    }
    GR_MAKE_REFLECTABLE(FileSource, out, filename);
};

} // namespace gr::packet_modem



#endif // _GR4_PACKET_MODEM_FILE_SOURCE

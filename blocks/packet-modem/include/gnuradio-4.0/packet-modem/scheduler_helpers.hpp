#ifndef _GR4_PACKET_MODEM_SCHEDULER_HELPERS
#define _GR4_PACKET_MODEM_SCHEDULER_HELPERS

#include <gnuradio-4.0/Graph.hpp>
#include <gnuradio-4.0/Scheduler.hpp>

#include <format>
#include <stdexcept>

namespace gr::packet_modem {

template <typename SchedulerT>
inline void init_scheduler(SchedulerT& sched, gr::Graph&& fg)
{
    if (auto ret = sched.exchange(std::move(fg)); !ret) {
        throw std::runtime_error(std::format("failed to initialize scheduler: {}", ret.error()));
    }
}

} // namespace gr::packet_modem

#endif // _GR4_PACKET_MODEM_SCHEDULER_HELPERS

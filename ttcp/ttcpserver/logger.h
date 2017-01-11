#ifndef __NETSTAT_TTCP_LOGGER__
#define __NETSTAT_TTCP_LOGGER__

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/expressions.hpp>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

#if defined(NDEBUG)
#define TTCP_BOOST_LOG_FUNCTION
#else
#define TTCP_BOOST_LOG_FUNCTION BOOST_LOG_FUNCTION();
#endif

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(ttcp_logger, src::severity_logger_mt<logging::trivial::severity_level>)
#define TTCP_LOGGER(sev) \
        TTCP_BOOST_LOG_FUNCTION \
        BOOST_LOG_SEV(ttcp_logger::get(), boost::log::trivial::sev) \

#endif // __NETSTAT_TTCP_LOGGER__
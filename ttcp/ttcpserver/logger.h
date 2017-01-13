#ifndef __NETSTAT_TTCP_LOGGER__
#define __NETSTAT_TTCP_LOGGER__

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/attributes/fallback_policy.hpp>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(ttcp_logger, src::severity_logger_mt<logging::trivial::severity_level>)

#if defined (NDEBUG)
#define LOG_FILE_LINE
#define ADD_FILE_LINE_ATTRIBUTES
#else
// Convert file path to only the filename
inline std::string path_to_filename(std::string path)
{
    return path.substr(path.find_last_of("/\\") + 1);
}
#define LOG_FILE_LINE \
        << logging::add_value("File", path_to_filename(__FILE__)) << logging::add_value("Line", __LINE__)
#define ADD_FILE_LINE_ATTRIBUTES \
        << '[' << expr::attr<std::string>("File") << ':' << expr::attr<int>("Line") << "] "
#endif

#define TTCP_LOGGER(sev) \
        BOOST_LOG_SEV(ttcp_logger::get(), boost::log::trivial::sev) \
        LOG_FILE_LINE \

#endif // __NETSTAT_TTCP_LOGGER__

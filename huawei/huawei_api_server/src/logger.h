#ifndef HUAWEI_API_SERVER_LOGGER_H_
#define HUAWEI_API_SERVER_LOGGER_H_

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/attributes/fallback_policy.hpp>
#include <boost/filesystem.hpp>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(server_logger, src::severity_logger_mt<logging::trivial::severity_level>)

#if defined (NDEBUG)
#define LOG_FILE_LINE
#define ADD_FILE_LINE_ATTRIBUTES
#else
#define LOG_FILE_LINE \
        << logging::add_value("File", boost::filesystem::basename(__FILE__)) << logging::add_value("Line", __LINE__)
#define ADD_FILE_LINE_ATTRIBUTES \
        << '[' << expr::attr<std::string>("File") << ':' << expr::attr<int>("Line") << "] "
#endif

#define SERVER_LOGGER(sev) \
        BOOST_LOG_SEV(server_logger::get(), boost::log::trivial::sev) \
        LOG_FILE_LINE \

#endif // HUAWEI_API_SERVER_LOGGER_H_

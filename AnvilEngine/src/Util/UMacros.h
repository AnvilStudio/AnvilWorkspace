#pragma once

#include "Profile.h"
#include "AnvLog/AnvLog.h"

#include <memory>
#include <vector>
#include <random>

namespace anv
{

// Commonly used types
// ====================================================================

// simplification for std::vector
template<typename _ty> 
using _vec = std::vector<_ty>;

// simplification for std::shared_ptr
template<typename _ty>
using _shared = std::shared_ptr<_ty>;

// simplification for std::unique_ptr
template<typename _ty>
using _unique = std::unique_ptr<_ty>;

// and so on...
template<typename _ty>
using _weak = std::weak_ptr<_ty>;

// Profiling
// ====================================================================

// profile a specific scope
#define ANV_PROFILE_SCOPE() Profiler _profile_scope(__func__);
#define ANV_PROFILE_SCOPE_NAME(scope) Profiler _profile_scope(scope);

// Logging
// ====================================================================

// #define ANV_LOG_INIT(info) anv_log::AnvLog::Init(info);

#define ANV_LOG_INFO(fmt, ...)  anv_log::AnvLog::LOG_INFO (fmt, ##__VA_ARGS__);
#define ANV_LOG_DEBUG(fmt, ...) anv_log::AnvLog::LOG_DEBUG(fmt, ##__VA_ARGS__);
#define ANV_LOG_WARN(fmt, ...)  anv_log::AnvLog::LOG_WARN (fmt, ##__VA_ARGS__);
#define ANV_LOG_ERROR(fmt, ...) anv_log::AnvLog::LOG_ERROR(fmt, ##__VA_ARGS__);
#define ANV_LOG_FATAL(fmt, ...) anv_log::AnvLog::LOG_FATAL(__func__, fmt, ##__VA_ARGS__);

// Tests And Checking
// ====================================================================

#define ANV_ASSERT(condition, message, ...)                                                    \
    do {                                                                                       \
        if (!(condition)) {                                                                    \
            auto msg = std::string("[%s:%d] [Assertion Failed]: ") + std::string(message);     \
            anv_log::AnvLog::LOG_CUST(anv_log::TermColor::TC_RED, anv_log::LogLevel::LL_FATAL, \
            msg, __FILE__, __LINE__, ##__VA_ARGS__);                                             \
        }                                                                                      \
    } while (false);

} // namespace anv

// ====================================================================

// Attributes
// ====================================================================

#define ANV_NO_DSCRD [[nodiscard]]
#define ANV_DEPRECATE(msg) [[deprecated(msg)]]

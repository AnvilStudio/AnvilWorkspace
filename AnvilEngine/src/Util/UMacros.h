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

// ====================================================================

namespace util
{
    inline std::string GenUID()
    {

        std::random_device rd;
        std::mt19937 mt(rd());

        const static _vec<char> alphaNum = {
            'a','A','0','b','B','1','c','C','2',
            'd','D','3','e','E','4','f','F','5',
            'g','G','6','h','H','7','i','I','8',
            'j','J','9','k','K','0','l','L','1',
            'm','M','2','n','N','3','o','O','4',
            'p','P','5','q','Q','6','r','R','7',
            's','S','8','t','T','9','u','U','0',
            'v','V','1','w','W','2','x','X','3',
            'y','Y','4','z','Z','5'
        };

        std::uniform_int_distribution<int> uni(0, alphaNum.size());

        std::string ID = "";

        // UID is 15 char long
        for (int i = 0; i < 15; i++)
        {
            ID.push_back(alphaNum[uni(mt)]);
        }

        return ID;
    }
}

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

//! ----------------------------------------------------------------------------
//! Log.h
//!
//! MTGDictionary is licensed under a
//! Creative Commons Attribution-NonCommercial 4.0 International License.
//! You should have received a copy of the license along with this
//! work. If not, see http://creativecommons.org/licenses/by-nc-sa/4.0/.
//!
//! (c) Copyright Dustin Hopper 2015 - hopper.dustin@gmail.com
//! ----------------------------------------------------------------------------

#pragma once

#include <fstream>
#include <iostream>

#define mtg_debug(x) (mtg::logConsole(__FILE__, __LINE__, mtg::LogSeverity::DEBUG, mtg::LogData<mtg::None>() << x))
#define mtg_info(x)  (mtg::logConsole(__FILE__, __LINE__, mtg::LogSeverity::INFO,  mtg::LogData<mtg::None>() << x))
#define mtg_warn(x)  (mtg::logConsole(__FILE__, __LINE__, mtg::LogSeverity::WARN,  mtg::LogData<mtg::None>() << x))
#define mtg_error(x) (mtg::logConsole(__FILE__, __LINE__, mtg::LogSeverity::ERROR, mtg::LogData<mtg::None>() << x))

namespace mtg
{
    struct None {};

    enum class LogSeverity { DEBUG, INFO, WARN, ERROR };
    static inline std::string LogSeverityToString(LogSeverity sev)
    {
        switch (sev)
        {
            case LogSeverity::DEBUG:
                return "DEBUG";
            case LogSeverity::INFO:
                return "INFO ";
            case LogSeverity::WARN:
                return "WARN ";
            case LogSeverity::ERROR:
                return "ERROR";
            default:
                return "INFO";
        }
    }

    template <typename First, typename Second>
    struct Pair
    {
        First first;
        Second second;
    };

    template <typename List>
    struct LogData
    {
        List list;
    };

    template <typename Begin, typename Value>
    LogData<Pair<Begin, const Value &>> operator<<(LogData<Begin> begin, const Value &value)
    {
        return {{ begin.list, value }};
    }

    template <typename Begin, uint64_t n>
    LogData<Pair<Begin, const int8_t *>> operator<<(LogData<Begin> begin, const int8_t(&value)[n])
    {
        return {{ begin.list, value }};
    }

    inline void printList(std::ostream &, mtg::None)
    {
    }

    template <typename Begin, typename Last>
    void printList(std::ostream &os, const Pair<Begin, Last> &data)
    {
        mtg::printList(os, data.first);
        os << data.second;
    }

    template <typename List>
    void logConsole(const char *file, int32_t line, LogSeverity sev, const LogData<List> &data)
    {
        std::string fstr(file);
        fstr = fstr.substr(fstr.find_last_of("/") + 1, fstr.length() - 1);
        std::cout << "[" << mtg::LogSeverityToString(sev).c_str() << "] - " << fstr.c_str() << " @ " << line << ": ";
        mtg::printList(std::cout, data.list);
        std::cout << "\n";
    }
}

#pragma once

#include <iomanip>
#include <pybind11/pybind11.h>

namespace py = pybind11;

class Timer
{
public:
    Timer(const std::string &message = "", int level = 0, bool timeit = true);
    void start(const std::string &message = "", int level = 0, bool timeit = true);
    void output() const;
    void stop() const;
    void reset(const std::string &message, bool timeit);

private:
    std::string message_;
    bool timeit_;
    int level_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

class Logger
{
public:
    Logger(int level) : level_(level) {}

    template <typename... Args>
    void info(Args &&...args) const
    {
        if (level_ >= 1)
        {
            py::print("[INFO]", std::forward<Args>(args)...);
        }
    }
    template <typename... Args>
    void debug(Args &&...args) const
    {
        if (level_ >= 2)
        {
            py::print("[DEBUG]", std::forward<Args>(args)...);
        }
    }
    template <typename... Args>
    void trace(Args &&...args) const
    {
        if (level_ >= 3)
        {
            py::print("[TRACE]", std::forward<Args>(args)...);
        }
    }

    // The following requires 'T' to be defined, add 'template<typename T>'
    template <typename T>
    void trace_xyz(std::string msg, T x, T y, T z, bool endline) const
    {
        if (level_ >= 3)
        {
            py::print("[TRACE]", msg, ":", "(", x, ",", y, ",", z, ")");
            if (endline)
                py::print();
        }
    }

private:
    int level_;
};
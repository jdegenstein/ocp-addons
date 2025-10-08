#include "utils.h"

Timer::Timer(const std::string &message, int level, bool timeit)
    : message_(message), level_(level), timeit_(timeit), start_(std::chrono::high_resolution_clock::now()) {}

void Timer::start(const std::string &message, int level, bool timeit)
{
    message_ = message;
    level_ = level;
    timeit_ = timeit;
    start_ = std::chrono::high_resolution_clock::now();
}
void Timer::output() const
{
    auto end = std::chrono::high_resolution_clock::now();
    double seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count() / 1000.0;
    std::stringstream stream;
    stream << std::fixed << std::setprecision(3) << std::setw(8) << seconds;
    std::string s = stream.str();
    std::string indent = "";
    for (int i = 0; i < level_; ++i)
        indent.append(" |");
    py::print(s, "sec: ", indent, message_);
}

void Timer::stop() const
{
    if (!timeit_)
        return;
    output();
}

void Timer::reset(const std::string &message, bool timeit)
{
    output();
    message_ = message;
    timeit_ = timeit;
    start_ = std::chrono::high_resolution_clock::now();
}

float *convert_to_float(const double *input, size_t size)
{
    float *result = new float[size];
    for (size_t i = 0; i < size; ++i)
    {
        result[i] = static_cast<float>(input[i]);
    }
    return result; // Caller is responsible for delete[]
}
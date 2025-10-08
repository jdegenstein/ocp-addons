#pragma once

#include <chrono>
#include <iomanip>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

/**
 * @brief A utility class for measuring and reporting execution time of code blocks.
 *
 * The Timer class provides functionality to measure elapsed time between start and stop points,
 * with optional message output and hierarchical level support for nested timing operations.
 *
 * @example
 * Timer timer("Processing data", 0, true);
 * // ... code to time ...
 * timer.stop();
 */

class Timer
{
public:
    /**
     * @brief Constructs a Timer object and optionally starts timing.
     *
     * @param message Optional message to display when outputting timing results (default: "")
     * @param level Hierarchical level for nested timing operations, affects output indentation (default: 0)
     * @param timeit Whether to actually perform timing measurements (default: true)
     */
    Timer(const std::string &message = "", int level = 0, bool timeit = true);

    /**
     * @brief Starts or restarts the timer with new parameters.
     *
     * @param message Optional message to display when outputting timing results (default: "")
     * @param level Hierarchical level for nested timing operations (default: 0)
     * @param timeit Whether to actually perform timing measurements (default: true)
     */
    void start(const std::string &message = "", int level = 0, bool timeit = true);

    /**
     * @brief Outputs the current timing information without stopping the timer.
     *
     * Displays the elapsed time along with the associated message and level formatting.
     */
    void output() const;

    /**
     * @brief Stops the timer and outputs the final timing results.
     *
     * Calculates and displays the total elapsed time from start to stop.
     */
    void stop() const;

    /**
     * @brief Resets the timer with a new message and timing flag.
     *
     * @param message New message to associate with this timer
     * @param level Hierarchical level for nested timing operations
     */
    void reset(const std::string &message, int level);

private:
    std::string message_;
    bool timeit_;
    int level_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

/**
 * @brief A logging utility class that provides different levels of logging output.
 *
 * The Logger class supports hierarchical logging levels where higher levels include
 * all lower level messages:
 * - Level 1: INFO messages
 * - Level 2: INFO + DEBUG messages
 * - Level 3: INFO + DEBUG + TRACE messages
 *
 * All logging output is directed through Python's print function via pybind11.
 */
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

/**
 * @brief Get human-readable type name for template debugging
 *
 * Utility function that demangles C++ type names for better readability
 * in debugging and error messages. Particularly useful for template
 * instantiation diagnostics.
 *
 * @tparam T The type to get the readable name for
 * @return String containing the demangled, human-readable type name
 */
template <typename T>
std::string readable_typename()
{
#ifdef _MSC_VER
    // MSVC: No standard demangling, just return name
    return typeid(T).name();

#else

#include <cxxabi.h>
    int status = 0;
    char *demangled = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
    std::string result = (status == 0 && demangled) ? demangled : typeid(T).name();
    if (demangled)
        free(demangled);
    return result;
#endif
}

/**
 * @brief Wraps a raw C++ array into a NumPy array with automatic memory management.
 *
 * This function creates a NumPy array that takes ownership of the provided raw pointer.
 * The memory will be automatically freed when the NumPy array is garbage collected.
 *
 * @tparam T The data type of the array elements (must be 4 bytes in size)
 * @param ptr Raw pointer to the array data (ownership transferred to NumPy)
 * @param n Number of elements in the array
 *
 * @return py::array_t<T> A 1D NumPy array wrapping the provided data
 *
 * @throws std::invalid_argument If sizeof(T) is not equal to 4 bytes
 *
 * @note The function assumes the pointer was allocated with new[] and will be
 *       deallocated with delete[] when the NumPy array is destroyed.
 * @warning The caller must not delete the pointer after calling this function,
 *          as ownership is transferred to the NumPy array.
 */

template <typename T>
py::array_t<T> wrap_numpy(T *ptr, int n)
{
    if (sizeof(T) != 4)
    {
        throw std::invalid_argument(
            std::string("ERROR: Wrong byte size " + std::to_string(sizeof(T)) + " of value '") +
            readable_typename<T>() +
            "', numpy array will be broken");
    }
    // Capsule will call delete[] when the array is GC’d
    py::capsule owner(ptr, [](void *p)
                      {
        T* t = reinterpret_cast<T*>(p);
        delete[] t; });
    // 1D array: shape [n], stride [sizeof(T)]
    return py::array_t<T>(
        {n},         // shape
        {sizeof(T)}, // int32 or float
        ptr,         // data pointer
        owner        // base/owner capsule
    );
}

/**
 * @brief Converts an array of double values to an array of float values.
 *
 * This function takes an input array of double-precision floating-point numbers
 * and converts them to single-precision floating-point numbers (float).
 *
 * @param input Pointer to the input array of double values to be converted
 * @param size Number of elements in the input array
 * @return float* Pointer to a newly allocated array of float values.
 *               The caller is responsible for freeing the returned memory.
 *               Returns nullptr if allocation fails or input is nullptr.
 *
 * @note The returned pointer must be freed by the caller using delete[] or free()
 *       depending on the allocation method used in the implementation.
 * @warning Conversion from double to float may result in precision loss.
 */

float *convert_to_float(const double *input, size_t size);
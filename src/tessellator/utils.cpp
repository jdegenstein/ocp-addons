#include "utils.h"

Timer::Timer(const std::string &message, int level, bool timeit)
    : message_(message), timeit_(timeit), level_(level), start_(std::chrono::high_resolution_clock::now()) {}

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

void Timer::reset(const std::string &message, int level)
{
    output();
    message_ = message;
    level_ = level;
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

std::string ShapeEnumToString(TopAbs_ShapeEnum type)
{
    switch (type)
    {
    case TopAbs_COMPOUND:
        return "TopAbs_COMPOUND";
    case TopAbs_COMPSOLID:
        return "TopAbs_COMPSOLID";
    case TopAbs_SOLID:
        return "TopAbs_SOLID";
    case TopAbs_SHELL:
        return "TopAbs_SHELL";
    case TopAbs_FACE:
        return "TopAbs_FACE";
    case TopAbs_WIRE:
        return "TopAbs_WIRE";
    case TopAbs_EDGE:
        return "TopAbs_EDGE";
    case TopAbs_VERTEX:
        return "TopAbs_VERTEX";
    case TopAbs_SHAPE:
        return "TopAbs_SHAPE";
    default:
        return "UNKNOWN";
    }
}

void PrintCheckStatuses(const TopoDS_Face &face, int index)
{
    BRepCheck_Analyzer checker(face);
    bool valid = checker.IsValid();
    py::print("face", index, "is", valid);

    TopAbs_ShapeEnum types[] = {TopAbs_VERTEX, TopAbs_EDGE, TopAbs_WIRE, TopAbs_FACE};

    for (TopAbs_ShapeEnum type : types)
    {
        for (TopExp_Explorer exp(face, type); exp.More(); exp.Next())
        {
            const TopoDS_Shape &subShape = exp.Current();

            Handle(BRepCheck_Result) result = checker.Result(subShape);
            if (!result.IsNull())
            {
                const BRepCheck_ListOfStatus &statusList = result->Status();
                py::print("SubShape type: ", ShapeEnumToString(type));
                for (BRepCheck_ListIteratorOfListOfStatus it(statusList); it.More(); it.Next())
                {
                    BRepCheck_Status status = it.Value();
                    std::ostringstream oss;
                    BRepCheck::Print(status, oss); // Outputs readable description
                    std::string statusString = oss.str();
                    py::print(statusString);
                }
            }
        }
    }
}
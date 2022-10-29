#include "helpers.h"
#include <fmt/format.h>

#ifdef DEBUG
#define OUTPUT ""
#else
#define OUTPUT ">/dev/null 2>&1"
#endif

namespace helpers
{

bool can_execute(std::string exename)
{
    auto cmd = fmt::format("which {} {}", exename, OUTPUT);
    return system(cmd.data()) == 0;
}

bool execute(std::string exename, std::string args)
{
    auto cmd = fmt::format("{} {} {}", exename, args, OUTPUT);
    return system(cmd.data()) == 0;
}

bool copy(std::string source, std::string destination)
{
    auto cmd = fmt::format("cp {} {} {}", source, destination, OUTPUT);
    return system(cmd.data()) == 0;
}

}

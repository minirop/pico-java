#include "helpers.h"
#include <fmt/format.h>

namespace helpers
{

bool can_execute(std::string exename)
{
    auto cmd = fmt::format("where.exe {}", exename);
    return system(cmd.data()) == 0;
}

bool execute(std::string exename, std::string args)
{
    auto cmd = fmt::format("{}.exe {}", exename, args);
    return system(cmd.data()) == 0;
}

bool copy(std::string source, std::string destination)
{
    auto cmd = fmt::format("copy {} {}", source, destination);
    return system(cmd.data()) == 0;
}

}

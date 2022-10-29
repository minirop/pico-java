#ifndef HELPERS_H
#define HELPERS_H

#include <string>

namespace helpers
{
    bool can_execute(std::string exename);
    bool execute(std::string exename, std::string args = "");
    bool copy(std::string source, std::string destination);
}

#endif // HELPERS_H

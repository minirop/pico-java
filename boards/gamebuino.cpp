#include "gamebuino.h"
#include <fmt/format.h>

namespace fs = std::filesystem;

void build_gamebuino(std::string project_name)
{
    if (system("which arduino-cli") != 0)
    {
        fmt::print("Could not find 'arduino-cli'.");
        return;
    }

    auto tempPath = fs::temp_directory_path();

    auto currentPath = fs::current_path();
    fs::current_path(tempPath);
    fs::create_directories(project_name + "/build");
    fs::current_path(tempPath / project_name);

    std::ofstream output_c(project_name + ".ino");

    output_c << "#include \"gamebuino-java.h\"\n";

    if (fields.size() || functions.size() > 1)
    {
        output_c << '\n';

        for (auto & field : fields)
        {
            output_c << field.type << " " << field.name << ";\n";
        }
        for (auto & func : functions)
        {
            output_c << getReturnType(func.descriptor) << " " << func.name << "(" << generateParameters(func.descriptor) << ");\n";
        }
    }

    for (auto & func : functions)
    {
        output_c << '\n';

        output_c << getReturnType(func.descriptor) << " " << func.name << "(" << generateParameters(func.descriptor) << ")\n{\n";

        int depth = 0;
        for (auto & inst : func.instructions)
        {
            if (inst.opcode == "}") --depth;
            if (inst.opcode.size())
            {
                output_c << std::string(depth + 1, '\t') << inst.opcode << '\n';
            }
            if (inst.opcode == "{") ++depth;
        }
        output_c << "}\n";
    }

    output_c.close();

    std::ofstream output_header("gamebuino-java.h");

    output_header << R"___(
#include <Gamebuino-Meta.h>

namespace gamebuino::gb {
    inline void begin() { ::gb.begin(); }
    inline void waitForUpdate() { ::gb.waitForUpdate(); }

    inline auto & display = ::gb.display;
}
)___";

    output_header.close();

    fs::current_path(tempPath / project_name);
    system("arduino-cli compile --fqbn gamebuino:samd:gamebuino_meta_native --output-dir build");
    system(fmt::format("cp build/{0}.ino.bin {1}/{0}.bin", project_name, currentPath.string()).data());
}

#include "gamebuino.h"
#include <fmt/format.h>

namespace fs = std::filesystem;
using namespace std::string_literals;

struct Resource
{
    std::string filename;
    Format format;
};

std::vector<Resource> resources;

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
    if (resources.size())
    {
        output_c << "#include \"resources.h\"\n";
    }

    output_c << "#if __has_include(\"userfile.h\")\n"
             << "#include \"userfile.h\"\n"
             << "#endif\n";

    if (fields.size())
    {
        output_c << '\n';

        output_c << "namespace " << project_name << " {\n";

        for (auto & field : fields)
        {
            output_c << '\t' << field.type;
            if (!field.init.has_value() && field.isArray)
            {
                output_c << '*';
            }
            output_c << " " << field.name;

            if (field.init.has_value())
            {
                if (field.isArray)
                {
                    output_c << "[]";
                }
                output_c << " = " << field.init.value();
            }

            output_c << ";\n";
        }

        output_c << "}\n";
    }

    if (functions.size())
    {
        output_c << '\n';

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

namespace gamebuino {
    namespace gb {
        inline void begin() { ::gb.begin(); }
        inline void waitForUpdate() { ::gb.waitForUpdate(); }
        inline void setFrameRate(int fps) { ::gb.setFrameRate(fps); }

        inline auto & display = ::gb.display;
        inline auto & buttons = ::gb.buttons;
    }

    namespace Button {
        inline auto A = BUTTON_A;
        inline auto B = BUTTON_B;
        inline auto LEFT = BUTTON_LEFT;
        inline auto RIGHT = BUTTON_RIGHT;
        inline auto UP = BUTTON_UP;
        inline auto DOWN = BUTTON_DOWN;
    }

    using Image = ::Gamebuino_Meta::Image;
}
)___";

    output_header.close();

    if (resources.size())
    {
        std::ofstream output_res_header("resources.h");

        bool hasCustomResources = false;
        for (auto & res : resources)
        {
            if (fs::exists(currentPath / res.filename))
            {
                output_res_header << "\n"
                                  << "const " << ((res.format == Format::Rgb565) ? "uint16_t" : "uint8_t")
                                  << " " << encode_filename(res.filename) << "[] = {\n";
                output_res_header << "};\n";
            }
            else
            {
                hasCustomResources = true;
            }
        }

        output_res_header.close();
    }

    if (fs::exists(currentPath / "userfile.h"))
    {
        fs::copy_file(currentPath / "userfile.h", fs::current_path() / "userfile.h", fs::copy_options::overwrite_existing);
    }
    else
    {
        fs::remove(fs::current_path() / "userfile.h");
    }

    fs::current_path(tempPath / project_name);
    system("arduino-cli compile --fqbn gamebuino:samd:gamebuino_meta_native --output-dir build");
    system(fmt::format("cp build/{0}.ino.bin {1}/{0}.bin", project_name, currentPath.string()).data());
}

void add_resource(std::string filename, Format format)
{
    resources.emplace_back(filename, format);
}

std::string encode_filename(std::string filename)
{
    boost::replace_all(filename, "."s, "_"s);
    boost::replace_all(filename, "/"s, "_"s);

    return filename;
}

#include "gamebuino.h"
#include <fmt/format.h>

struct Resource
{
    std::string filename;
    Format format;
};

std::vector<Resource> resources;

void build_gamebuino(std::string project_name, std::vector<ClassFile> files)
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

    for (auto & file : files)
    {
        file.generate(files);
    }

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
        inline auto & frameCount = ::gb.frameCount;
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
        std::ofstream output_res_header(RESOURCES_FILE);

        for (auto & res : resources)
        {
            if (fs::exists(currentPath / res.filename))
            {
                output_res_header << "\n"
                                  << "const " << ((res.format == Format::Rgb565) ? "uint16_t" : "uint8_t")
                                  << " " << encode_filename(res.filename) << "[] = {\n";
                output_res_header << "};\n";
            }
        }

        output_res_header.close();
    }

    if (fs::exists(currentPath / RESOURCES_FILE))
    {
        fs::copy_file(currentPath / RESOURCES_FILE, fs::current_path() / RESOURCES_FILE, fs::copy_options::overwrite_existing);
    }
    else
    {
        fs::remove(fs::current_path() / RESOURCES_FILE);
    }

    if (fs::exists(currentPath / USER_FILE))
    {
        fs::copy_file(currentPath / USER_FILE, fs::current_path() / USER_FILE, fs::copy_options::overwrite_existing);
    }
    else
    {
        fs::remove(fs::current_path() / USER_FILE);
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

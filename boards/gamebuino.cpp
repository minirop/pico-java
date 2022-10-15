#include "gamebuino.h"
#include <fmt/format.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef _WIN64
#ifdef DEBUG
#define OUTPUT ""
#else
#define OUTPUT ">/dev/null 2>&1"
#endif
#endif

struct Resource
{
    std::string filename;
    Format format;
    int xcount = 1;
    int ycount = 1;
    int loop = 0;
};

std::vector<Resource> resources;

void encode_file(std::ofstream & stream, Resource res);

void build_gamebuino(std::string project_name, std::vector<ClassFile> files)
{
#ifndef _WIN64
    if (system("which arduino-cli " OUTPUT) != 0)
    {
        fmt::print("Could not find 'arduino-cli'.");
        return;
    }
#endif

    auto tempPath = fs::temp_directory_path();

    auto currentPath = fs::current_path();
    fs::current_path(tempPath);
    fs::create_directories(project_name + "/build");
    fs::current_path(tempPath / project_name);

    for (auto & file : files)
    {
        file.generate(files, Board::Gamebuino);
    }

    std::ofstream output_header("gamebuino-java.h");

    output_header << R"___(
#include <Gamebuino-Meta.h>

namespace std
{
    using string = String;

    template <typename T>
    inline string to_string(T t)
    {
        return string(t);
    }
}

namespace gamebuino {
    using Rotation = Gamebuino_Meta::Rotation;
    using Color = Gamebuino_Meta::Color;

    namespace gb {
        inline void begin() { ::gb.begin(); }
        inline void waitForUpdate() { ::gb.waitForUpdate(); }
        inline void setFrameRate(int fps) { ::gb.setFrameRate(fps); }
        inline bool update() { return ::gb.update(); }
        inline void setScreenRotation(gamebuino::Rotation r) { ::gb.setScreenRotation(r); }

        inline auto & display = ::gb.display;
        inline auto & buttons = ::gb.buttons;
        inline auto & sound = ::gb.sound;
        inline auto & save = ::gb.save;
        inline auto & gui = ::gb.gui;
        inline auto & frameCount = ::gb.frameCount;
    }

    namespace Button {
        inline auto A = BUTTON_A;
        inline auto B = BUTTON_B;
        inline auto LEFT = BUTTON_LEFT;
        inline auto RIGHT = BUTTON_RIGHT;
        inline auto UP = BUTTON_UP;
        inline auto DOWN = BUTTON_DOWN;
        inline auto MENU = BUTTON_MENU;
    }

    using Image = ::Gamebuino_Meta::Image;
}

namespace std::std {
    int random(int a, int b)
    {
        return ::random(a, b);
    }
}
)___";

    output_header.close();

    if (resources.size())
    {
        std::ofstream output_res_header(RESOURCES_FILE + ".h"s);

        output_res_header << "#include <cstdint>\n";

        for (auto & res : resources)
        {
            if (fs::exists(currentPath / res.filename))
            {
                output_res_header << "\n"
                                  << "extern const " << ((res.format == Format::Rgb565) ? "uint16_t" : "uint8_t")
                                  << " " << encode_filename(res.filename) << "[];\n";
            }
        }

        output_res_header.close();

        std::ofstream output_res_source(RESOURCES_FILE + ".ino"s);

        output_res_source << "#include <cstdint>\n";

        for (auto & res : resources)
        {
            if (fs::exists(currentPath / res.filename))
            {
                output_res_source << "\n"
                                  << "const " << ((res.format == Format::Rgb565) ? "uint16_t" : "uint8_t")
                                  << " " << encode_filename(res.filename) << "[] = { ";
                res.filename = currentPath.string() + "/" + res.filename;
                encode_file(output_res_source, res);
                output_res_source << " };\n";
            }
        }

        output_res_source.close();
    }
    else
    {
        fs::remove(fs::current_path() / (RESOURCES_FILE + ".h"s));
        fs::remove(fs::current_path() / (RESOURCES_FILE + ".ino"s));
    }

    copyUserFiles(currentPath);

    auto ret = system(
                "arduino-cli"
            #ifdef _WIN64
                ".exe"
            #endif
                " compile --fqbn gamebuino:samd:gamebuino_meta_native --output-dir build"
            #ifndef _WIN64
                " " OUTPUT
            #endif
                );
    if (ret != 0)
    {
        fmt::print("Error during the generation of the .bin file!");
        return;
    }

    ret = system(fmt::format(
                 #ifdef _WIN64
                     "copy"
                 #else
                     "cp"
                 #endif
                     " build{2}{0}.ino.bin {1}{2}{0}.bin"
                 #ifndef _WIN64
                     " " OUTPUT
                 #endif
                     , project_name, currentPath.string(),
                 #ifdef _WIN64
                     "\\"
                 #else
                     "/"
                 #endif
                     ).data());
    if (ret == 0)
    {
        fmt::print("Success!");
    }
    else
    {
        fmt::print("Failure!");
    }
}

void add_resource(std::string filename, Format format, int yframes, int xframes, int loop)
{
    resources.emplace_back(filename, format, yframes, xframes, loop);
}

std::string encode_filename(std::string filename)
{
    boost::replace_all(filename, "."s, "_"s);
    boost::replace_all(filename, "/"s, "_"s);

    return filename;
}

uint8_t findNearestIndex(uint16_t colour)
{
    return colour & 0xF;
}

uint16_t rgb32_to_565(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (a < 128)
    {
        r = 255;
        g = 0;
        b = 255;
    }

    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

void encode_file(std::ofstream & stream, Resource res)
{
    int x, y, comp;
    if (!stbi_info(res.filename.data(), &x, &y, &comp))
    {
        throw fmt::format("Can't read '{}'.", res.filename);
    }

    int width = x / res.xcount;
    int height = y / res.ycount;
    int framesCount = res.xcount * res.ycount;

    stream << width << ", " << height << ", ";
    if (res.format == Format::Indexed)
    {
        stream << (framesCount & 0xFF) << ", " << ((framesCount >> 8) & 0xFF);
    }
    else
    {
        stream << framesCount;
    }
    stream << ", " << res.loop
           << ", " << ((res.format == Format::Indexed) ? "0xFF" : fmt::format("0x{:x}", rgb32_to_565(255, 0, 255, 255)))
           << ", " << std::to_underlying(res.format);

    auto data = stbi_load(res.filename.data(), &x, &y, &comp, comp);

    assert(comp == 4);

    for (int iy = 0; iy < res.ycount; ++iy)
    {
        for (int ix = 0; ix < res.xcount; ++ix)
        {
            int fx = ix * width;
            int fy = iy * height;

            for (int dy = fy; dy < fy + height; ++dy)
            {
                auto ptr = &data[dy * x * 4 + fx];
                for (int dx = fx; dx < fx + width; ++dx)
                {
                    if (res.format == Format::Indexed)
                    {
                        auto r1 = *ptr++;
                        auto g1 = *ptr++;
                        auto b1 = *ptr++;
                        [[maybe_unused]] auto a1 = *ptr++;

                        auto r2 = *ptr++;
                        auto g2 = *ptr++;
                        auto b2 = *ptr++;
                        [[maybe_unused]] auto a2 = *ptr++;

                        uint16_t colour1 = ((r1 >> 3) << 11) | ((g1 >> 2) << 5) | (b1 >> 3);
                        uint16_t colour2 = ((r2 >> 3) << 11) | ((g2 >> 2) << 5) | (b2 >> 3);
                        stream << ", " << fmt::format("0x{:x}{:x}", findNearestIndex(colour1), findNearestIndex(colour2));
                    }
                    else
                    {
                        auto r = *ptr++;
                        auto g = *ptr++;
                        auto b = *ptr++;
                        auto a = *ptr++;

                        uint16_t colour = rgb32_to_565(r, g, b, a);
                        stream << ", " << fmt::format("0x{:x}", colour);
                    }
                }
            }
        }
    }
}

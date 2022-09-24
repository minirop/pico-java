#include "pico.h"
#include <globals.h>
#include <fstream>
#include <filesystem>
#include <fmt/format.h>

namespace fs = std::filesystem;

std::set<std::string> usedFieldNames;

std::string get_cmake_board_name(Board board);
std::string generateParameters(std::string descriptor);

void build_pico(std::string project_name, Board board)
{
    if (!getenv("PICO_SDK_PATH"))
    {
        fmt::print("$PICO_SDK_PATH is not set nor accessible. Aborting.\n");
        return;
    }

    auto currentPath = fs::current_path();
    auto tempPath = fs::temp_directory_path();
    std::string tempDir = "java-pico";

    std::string libs;
    fs::current_path(tempPath);
    fs::create_directories(tempDir + "/build");
    fs::current_path(tempPath / tempDir);

    std::ofstream output_c("main.cpp");

    output_c << "#include \"pico-java.h\"\n"
                  "\n";

    output_c << "namespace " << project_name << "\n{\n";
    for (auto & field : fields)
    {
        output_c << '\t' << field.type << " " << field.name << ";\n";
    }
    for (auto & func : functions)
    {
        if (func.name != "main")
        {
            output_c << '\t' << getReturnType(func.descriptor) << " " << func.name << "(" << generateParameters(func.descriptor) << ");\n";
        }
    }
    output_c << "}\n\n";

    bool has_static_init = false;
    for (auto & func : functions)
    {
        if (func.name == "static_init")
        {
            has_static_init = true;
        }
    }

    for (auto & func : functions)
    {
        bool isMain = func.name == "main";

        if (!isMain)
        {
            output_c << "namespace " << project_name << " {\n";
        }

        output_c << (isMain ? "int" : getReturnType(func.descriptor)) << " " << func.name << "(" << (isMain ? "" : generateParameters(func.descriptor)) << ")\n{\n";
        if (isMain && has_static_init)
        {
            output_c << "\t" << project_name << "::static_init();\n";
        }
        for (auto & inst : func.instructions)
        {
            if (inst.label.has_value())
            {
                output_c << inst.label.value() << ":\n";
            }

            output_c << '\t' << inst.opcode << '\n';
        }
        output_c << "}\n";

        if (!isMain)
        {
            output_c << "}\n";
        }

        output_c << '\n';
    }

    if (board == Board::PicoW)
    {
        libs = "pico_cyw43_arch_none";
    }

    output_c.close();

    std::ofstream output_cmake("CMakeLists.txt");

    output_cmake << fmt::format(R"___(
cmake_minimum_required(VERSION 3.12)

set(PICO_BOARD "{2}")

include($ENV{{PICO_SDK_PATH}}/external/pico_sdk_import.cmake)

project({0})

pico_sdk_init()

add_executable({0}
    main.cpp
)

pico_add_extra_outputs({0})

target_link_libraries({0} pico_stdlib {1})

)___", project_name, libs, get_cmake_board_name(board));

    output_cmake.close();

    std::ofstream output_header("pico-java.h");

    output_header << R"___(
#include "pico/stdlib.h"

namespace pico
{
    namespace stdio
    {
        inline void init_all()
        {
            stdio_init_all();
        }
    }

    namespace gpio
    {
        static inline int INPUT = GPIO_IN;
        static inline int OUTPUT = GPIO_OUT;
        static inline int IRQ_EDGE_RISE = GPIO_IRQ_EDGE_RISE;
        static inline int IRQ_EDGE_FALL = GPIO_IRQ_EDGE_FALL;

        inline void init(int pin)
        {
            gpio_init(pin);
        }

        inline void set_dir(int pin, int dir)
        {
            gpio_set_dir(pin, dir);
        }

        inline void put(int pin, int value)
        {
            gpio_put(pin, value);
        }

        inline bool get(int pin)
        {
            return gpio_get(pin);
        }

        inline void pull_up(int pin)
        {
            gpio_pull_up(pin);
        }

        inline void pull_down(int pin)
        {
            gpio_pull_down(pin);
        }

        inline void set_input_enabled(int pin, bool enabled)
        {
            gpio_set_input_enabled(pin, enabled);
        }

        inline void set_irq_enabled_with_callback(int pin, int events, bool enabled, gpio_irq_callback_t callback)
        {
            gpio_set_irq_enabled_with_callback(pin, events, enabled, callback);
        }

        inline void set_mask(int pin)
        {
            gpio_set_mask(pin);
        }

        inline void clr_mask(int pin)
        {
            gpio_clr_mask(pin);
        }
    }

    namespace time
    {
        inline void sleep_ms(int ms)
        {
            ::sleep_ms(ms);
        }
    }
}
)___";

    if (board == Board::PicoW)
    {
        output_header << R"___(
#include "pico/cyw43_arch.h"

namespace pico
{
    namespace wifi
    {
        int LED_PIN = CYW43_WL_GPIO_LED_PIN;

        inline void init()
        {
            cyw43_arch_init();
        }

        inline void gpio_put(int pin, int status)
        {
            cyw43_arch_gpio_put(pin, status);
        }
    }
}
)___";
    }

    output_header.close();

    fs::current_path(tempPath / tempDir / "build");
    system("cmake ..");
    system("make");
    system(fmt::format("cp {}.uf2 {}", project_name, currentPath.string()).data());
}

std::string get_cmake_board_name(Board board)
{
    if (board == Board::Pico)         return "pico";
    if (board == Board::PicoW)        return "pico_w";
    if (board == Board::Tiny2040)     return "pimoroni_tiny2040";
    if (board == Board::Tiny2040_2mb) return "pimoroni_tiny2040_2mb";

    assert(false);
}

std::string generateParameters(std::string descriptor)
{
    std::string ret;

    int count = 0;
    assert(descriptor[0] == '(');
    for (size_t index = 1; index < descriptor.size(); ++index)
    {
        switch (descriptor[index])
        {
        case ')':
            index = descriptor.size();
            break;
        case 'L':
            while (descriptor[index] != ';') ++index;
            assert(false);
            break;
        case 'I':
            ret += fmt::format(", int ilocal_{}", count);
            ++count;
            break;
        case 'Z':
            ret += fmt::format(", int ilocal_{}", count);
            ++count;
            break;
        default:
            assert(false);
        }
    }

    if (ret.size() > 0)
    {
        ret = ret.substr(2);
    }
    return ret;
}

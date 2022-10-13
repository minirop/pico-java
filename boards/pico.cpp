#include "pico.h"
#include "globals.h"
#include <fstream>
#include <filesystem>
#include <fmt/format.h>

std::set<std::string> usedFieldNames;

std::string get_cmake_board_name(Board board);

void build_pico(std::string project_name, Board board, std::vector<ClassFile> files)
{
    if (!getenv("PICO_SDK_PATH"))
    {
        fmt::print("$PICO_SDK_PATH is not set nor accessible. Aborting.\n");
        return;
    }

    if (board == Board::Badger2040)
    {
        if (!getenv("PIMORONI_PICO_PATH"))
        {
            fmt::print("$PIMORONI_PICO_PATH is not set nor accessible. Aborting.\n");
            return;
        }
    }

    auto tempPath = fs::temp_directory_path();
    std::string tempDir = "pico-" + project_name;

    auto currentPath = fs::current_path();
    fs::current_path(tempPath);
    fs::create_directories(tempDir + "/build");
    fs::current_path(tempPath / tempDir);

    for (auto & file : files)
    {
        file.generate(files, board);
    }

    std::ofstream output_cmake("CMakeLists.txt");

    std::string libs;
    if (board == Board::PicoW)
    {
        libs = "pico_cyw43_arch_none";
    }

    output_cmake << "cmake_minimum_required(VERSION 3.12)\n"
                 << fmt::format("set(PICO_BOARD \"{}\")\n", get_cmake_board_name(board))
                 << "include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)\n";
    if (board == Board::Badger2040)
    {
        output_cmake << "include($ENV{PIMORONI_PICO_PATH}/pimoroni_pico_import.cmake)\n"
                     << "include($ENV{PIMORONI_PICO_PATH}/libraries/bitmap_fonts/bitmap_fonts.cmake)\n"
                     << "include($ENV{PIMORONI_PICO_PATH}/libraries/hershey_fonts/hershey_fonts.cmake)\n"
                     << "include($ENV{PIMORONI_PICO_PATH}/drivers/uc8151_legacy/uc8151_legacy.cmake)\n"
                     << "include($ENV{PIMORONI_PICO_PATH}/libraries/badger2040/badger2040.cmake)\n";
    }
    output_cmake << fmt::format("project({})\n", project_name)
                 << "pico_sdk_init()\n"
                 << fmt::format("add_executable({}", project_name);
    for (auto & file : files)
    {
        output_cmake << " " << file.fileName << ".cpp";
    }
    output_cmake << ")\n"
                 << fmt::format("pico_add_extra_outputs({})\n", project_name);
    if (board == Board::Badger2040)
    {
        output_cmake << fmt::format("target_include_directories({} PRIVATE $ENV{{PIMORONI_PICO_PATH}} $ENV{{PIMORONI_PICO_PATH}}/libraries/badger2040)\n", project_name);
        libs = "badger2040 hardware_spi";
    }
    output_cmake << fmt::format("target_link_libraries({} pico_stdlib {})\n", project_name, libs);
    output_cmake.close();

    std::ofstream output_header("pico-java.h");

    output_header << R"___(
#ifndef PICO_JAVA_H
#define PICO_JAVA_H

#include "pico/stdlib.h"
#include <string>

namespace pico
{
    namespace stdio
    {
        inline static void init_all()
        {
            stdio_init_all();
        }
    }

    namespace gpio
    {
        static inline auto INPUT = GPIO_IN;
        static inline auto OUTPUT = GPIO_OUT;
        static inline auto IRQ_EDGE_RISE = GPIO_IRQ_EDGE_RISE;
        static inline auto IRQ_EDGE_FALL = GPIO_IRQ_EDGE_FALL;

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
    else if (board == Board::Badger2040)
    {
        output_header << R"___(
#include "badger2040.hpp"

namespace pimoroni
{
    static inline Badger2040 badger2040;

    namespace badger
    {
        inline void init()
        {
            badger2040.init();
        }

        inline void halt()
        {
            badger2040.halt();
        }

        inline void update()
        {
            badger2040.update();
        }

        inline void clear()
        {
            badger2040.clear();
        }

        inline void wait_for_press()
        {
            badger2040.wait_for_press();
        }

        inline void led(int brightness)
        {
            badger2040.led(brightness);
        }

        inline void thickness(int size)
        {
            badger2040.thickness(size);
        }

        inline void pen(int colour)
        {
            badger2040.pen(colour);
        }

        inline bool pressed_to_wake(int pin)
        {
            return badger2040.pressed_to_wake(pin);
        }

        inline bool is_busy()
        {
            return badger2040.is_busy();
        }

        inline void text(std::string string, int x, int y, float s)
        {
            badger2040.text(string, x, y, s);
        }

        static inline auto A = badger2040.A;
        static inline auto B = badger2040.B;
        static inline auto C = badger2040.C;
        static inline auto D = badger2040.D;
        static inline auto E = badger2040.E;
    }
}
)___";
    }
    else if (board == Board::Tiny2040 || board == Board::Tiny2040_2mb)
    {
        output_header << R"___(
namespace pimoroni
{
    namespace tiny2040
    {
        static inline auto LED_R_PIN = TINY2040_LED_R_PIN;
        static inline auto LED_G_PIN = TINY2040_LED_G_PIN;
        static inline auto LED_B_PIN = TINY2040_LED_B_PIN;
    }
})___";
    }

    output_header << "#endif\n";

    output_header.close();

    copyUserFiles(currentPath);

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
    if (board == Board::Badger2040)   return "pimoroni_badger2040";
    if (board == Board::Picosystem)   return "pimoroni_picosystem";

    throw fmt::format("Unknown board {}!", std::to_underlying(board));
}

#include "picosystem.h"

void build_picosystem(std::string project_name, std::vector<ClassFile> files)
{
    if (!getenv("PICO_SDK_PATH"))
    {
        fmt::print("$PICO_SDK_PATH is not set nor accessible. Aborting.\n");
        return;
    }

    if (!getenv("PICOSYSTEM_SDK_PATH"))
    {
        fmt::print("$PICOSYSTEM_SDK_PATH is not set nor accessible. Aborting.\n");
        return;
    }

    auto tempPath = fs::temp_directory_path();
    std::string tempDir = "picosystem-" + project_name;

    auto currentPath = fs::current_path();
    fs::current_path(tempPath);
    fs::create_directories(tempDir + "/build");
    fs::current_path(tempPath / tempDir);

    for (auto & file : files)
    {
        file.generate(files, Board::Picosystem);
    }

    std::ofstream output_cmake("CMakeLists.txt");

    output_cmake << R"___(
cmake_minimum_required(VERSION 3.12)

set(PICO_BOARD "pimoroni_picosystem")

# Set your project name here
set(PROJECT_NAME
)___";
    output_cmake << project_name << "\n";
    output_cmake << R"___(
)

# Set your project sources here
set(PROJECT_SOURCES
)___";

    for (auto & file : files)
    {
        output_cmake << "    " << file.fileName << ".cpp\n";
    }

    if (fs::exists(currentPath / (USER_FILE + ".cpp"s)))
    {
        output_cmake << "    " << USER_FILE << ".cpp\n";
    }

    if (fs::exists(currentPath / (RESOURCES_FILE + ".cpp"s)))
    {
        output_cmake << "    " << RESOURCES_FILE << ".cpp\n";
    }

    output_cmake << R"___(
)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# set up project
project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# Find the PicoSystem library
include($ENV{PICOSYSTEM_SDK_PATH}/picosystem-config.cmake)

# Create the output target
picosystem_executable(
  ${PROJECT_NAME}
  ${PROJECT_SOURCES}
)
)___";

    for (auto & file : files)
    {
        for (auto & str : file.rawCMake)
        {
            output_cmake << str << '\n';
        }
    }

    output_cmake.close();

    std::ofstream output_header("picosystem-java.h");

    output_header << R"___(
#ifndef PICOSYSTEM_JAVA_H
#define PICOSYSTEM_JAVA_H

#include "picosystem.hpp"

namespace pimoroni {
    namespace picosystem {
        using namespace ::picosystem;
    }

    class buffer : public picosystem::buffer_t {
    public:
        buffer(int32_t w, int32_t h, const uint16_t *data) : buffer_t { .w = w, .h = h, .data = (picosystem::color_t *)data, .alloc = false } {
        }
    };
}

#endif
)___";

    output_header.close();

    copyUserFiles(currentPath);

    //return;
    fs::current_path(tempPath / tempDir / "build");
    system("cmake ..");
    system("make");
    system(fmt::format("cp {}.uf2 {}", project_name, currentPath.string()).data());
}

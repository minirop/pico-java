#include "globals.h"
#include "boards/pico.h"
#include "boards/gamebuino.h"
#include <bit>

using namespace std::string_literals;

u4 countArgs(std::string str)
{
    int count = 0;
    if (str[0] != '(')
    {
        throw fmt::format("Invalid descriptor. Should start with '(', got '{}'!", str[0]);
    }

    for (size_t index = 1; index < str.size(); ++index)
    {
        switch (str[index])
        {
        case ')':
            index = str.size();
            break;
        case 'L':
            ++count;
            while (str[index] != ';') ++index;
            break;
        case 'I':
        case 'Z':
        case 'F':
        case 'B':
            ++count;
            break;
        case '[':
            break;
        default:
            throw fmt::format("Invalid or unhandled type used as a parameter type: '{}'.", str[index]);
        }
    }

    return count;
}

attribute_info readAttribute(Buffer & buffer);

std::vector<Instruction> convertBytecode(Buffer & buffer, std::string function, u2 depth);
void initializeFields(Buffer & buffer);
Board getBoardTypeFromString(std::string board_name);

int main(int argc, char** argv)
{
    std::vector<std::string> javaFiles;
    for (auto const& dir_entry : std::filesystem::directory_iterator{"."})
    {
        if (dir_entry.is_regular_file())
        {
            auto filename_entry = dir_entry.path().filename().string();
            if (filename_entry.ends_with(".java"))
            {
                javaFiles.push_back(filename_entry);
            }
        }
    }

    if (!javaFiles.size())
    {
        fmt::print("No .java file detected. Aborting.\n");
        return 0;
    }

    for (auto & file : javaFiles)
    {
        int r = system(fmt::format("javac {}", file).data());

        if (r != 0)
        {
            fmt::print("{} has errors. Aborting.\n", file);
            return 0;
        }
    }

    std::string project_name, board_name;
    std::vector<ClassFile> classFiles;
    for (auto & file : javaFiles)
    {
        classFiles.push_back(ClassFile(file));
        if (classFiles.back().hasBoard())
        {
            project_name = classFiles.back().fileName;
            board_name = classFiles.back().boardName();
            break;
        }
    }

    auto board = getBoardTypeFromString(board_name);
    if (board == Board::Gamebuino)
    {
        build_gamebuino(project_name, classFiles);
    }
    else
    {
        build_pico(project_name, getBoardTypeFromString(board_name), classFiles);
    }

    return 0;
}

u1 read8(Buffer & buffer)
{
    auto byte = buffer.front();
    buffer.erase(buffer.begin());
    return byte;
}

u2 read16(Buffer & buffer)
{
    auto MSB = read8(buffer);
    auto LSB = read8(buffer);

    return MSB << 8 | LSB;
}

u4 read32(Buffer & buffer)
{
    auto MSB = read16(buffer);
    auto LSB = read16(buffer);

    return MSB << 16 | LSB;
}

s1 reads8(Buffer & buffer)
{
    return r8();
}

s2 reads16(Buffer & buffer)
{
    return r16();
}

s4 reads32(Buffer & buffer)
{
    return r32();
}

std::string getReturnType(std::string descriptor)
{
    auto paren = descriptor.find(')');
    auto type = descriptor.substr(paren + 1);

    if (type == "V")
    {
        return "void";
    }

    if (type == "Z")
    {
        return "bool";
    }

    if (type  == "I")
    {
        return "int";
    }

    throw fmt::format("Invalid or unhandled type used as a return type: '{}'.", type);
}

Board getBoardTypeFromString(std::string board_name)
{
    std::transform(begin(board_name), end(board_name), begin(board_name), [](auto c) { return std::tolower(c); });

    if (board_name == "pico")         return Board::Pico;
    if (board_name == "picow")        return Board::PicoW;
    if (board_name == "tiny2040")     return Board::Tiny2040;
    if (board_name == "tiny2040_2mb") return Board::Tiny2040_2mb;
    if (board_name == "badger2040")   return Board::Badger2040;
    if (board_name == "gamebuino")    return Board::Gamebuino;

    throw fmt::format("Invalid board: '{}'.", board_name);
}

std::string generateParameters(std::string descriptor)
{
    std::string ret;

    int count = 0;
    if (descriptor[0] != '(')
    {
        throw fmt::format("Invalid descriptor. Should start with '(', got '{}'!", descriptor[0]);
    }

    int arrayCount = 0;
    for (size_t index = 1; index < descriptor.size(); ++index)
    {
        switch (descriptor[index])
        {
        case ')':
            index = descriptor.size();
            break;
        case 'L':
            while (descriptor[index] != ';') ++index;
            throw fmt::format("classes are not supported as function arguments.");
            break;
        case 'I':
        case 'Z':
            ret += fmt::format(", int {}local_{}", std::string(arrayCount, '*'), count);
            arrayCount = 0;
            ++count;
            break;
        case '[':
            ++arrayCount;
            break;
        default:
            throw fmt::format("Invalid character in descriptor: '{}'.", descriptor[index]);
        }
    }

    if (ret.size() > 0)
    {
        ret = ret.substr(2);
    }
    return ret;
}

std::string javaToCpp(std::string name)
{
    return boost::replace_all_copy(name, "/"s, "::"s);
}

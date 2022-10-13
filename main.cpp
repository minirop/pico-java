#include "globals.h"
#include "boards/pico.h"
#include "boards/gamebuino.h"
#include "boards/picosystem.h"
#include <bit>

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
        case 'S':
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

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
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

    try
    {
        std::string project_name, board_name;
        for (auto & file : javaFiles)
        {
            ClassFile tmp(file, ""s, true);
            if (tmp.hasBoard())
            {
                project_name = tmp.fileName;
                board_name = tmp.boardName();
            }
        }

        for (const fs::directory_entry& dir_entry :
                fs::recursive_directory_iterator("."))
        {
            if (dir_entry.is_regular_file() && dir_entry.path().extension().string() == ".class")
            {
                ClassFile tmp(dir_entry.path().string(), ""s, true);
                ClassFile::partialClasses.push_back(tmp);
            }
        }

        std::vector<ClassFile> classFiles;
        for (auto & file : javaFiles)
        {
            classFiles.push_back(ClassFile(file, project_name));
        }

        auto board = getBoardTypeFromString(board_name);
        if (board == Board::Gamebuino)
        {
            build_gamebuino(project_name, classFiles);
        }
        else if (board == Board::Picosystem)
        {
            build_picosystem(project_name, classFiles);
        }
        else
        {
            build_pico(project_name, getBoardTypeFromString(board_name), classFiles);
        }
    }
    catch (const std::string & str)
    {
        fmt::print("{}\n", str);
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

    if (type == "I")
    {
        return "int";
    }

    if (type.starts_with("L") && type.ends_with(";"))
    {
        auto jt = type.substr(1, type.size() - 2);

        if (jt == "java/lang/String")
        {
            return "std::string";
        }
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
    if (board_name == "picosystem")    return Board::Picosystem;

    throw fmt::format("Invalid board: '{}'.", board_name);
}

std::string generateParameters(std::string descriptor, std::vector<u1> flags, bool isMethod)
{
    std::string ret;

    int count = isMethod ? 1 : 0;
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
        {
            std::string type;
            ++index;
            while (descriptor[index] != ';')
            {
                type += descriptor[index];
                ++index;
            }

            if (flags[count] & POINTER_TYPE) ++arrayCount;

            if (type == "java/lang/String")
            {
                ret += fmt::format(", std::string {}local_{}", std::string(arrayCount, '*'), count);
            }
            else if (type == "pimoroni/buffer")
            {
                ret += fmt::format(", pimoroni::buffer {}local_{}", std::string(arrayCount, '*'), count);
            }
            else
            {
                throw fmt::format("classes are not supported as function arguments.");
            }
            break;
        }
        case 'I':
        case 'Z':
        {
            ret += fmt::format(", {} {}local_{}", getTypeFromDescriptor(descriptor[index]+""s, flags[count]), std::string(arrayCount, '*'), count);
            arrayCount = 0;
            ++count;
            break;
        }
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

void copyUserFiles(fs::path currentPath)
{
    for (auto ext : { ".h"s, ".cpp"s })
    {
        if (fs::exists(currentPath / (USER_FILE + ext)))
        {
            fs::copy_file(currentPath / (USER_FILE + ext), fs::current_path() / (USER_FILE + ext), fs::copy_options::overwrite_existing);
        }
        else
        {
            fs::remove(fs::current_path() / (USER_FILE + ext));
        }
    }
}

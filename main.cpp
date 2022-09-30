#include "globals.h"
#include "lineanalyser.h"
#include "boards/pico.h"

using namespace std::string_literals;

u4 countArgs(std::string str)
{
    int count = 0;
    assert(str[0] == '(');
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
            ++count;
            break;
        default:
            assert(false);
        }
    }

    return count;
}

attribute_info readAttribute(Buffer & buffer);

std::vector<Instruction> convertBytecode(Buffer & buffer, std::string function, u2 depth);
void initializeFields(Buffer & buffer);
std::string getTypeFromDescriptor(std::string descriptor);
Board getBoardTypeFromString(std::string board_name);

ConstantPool constantPool;
std::vector<FieldData> fields;
std::vector<FunctionData> functions;
std::vector<std::string> callbacksMethods;

std::string getStringFromUtf8(int index)
{
    auto b = std::get<Utf8>(constantPool[index]).bytes;
    return std::string { begin(b), end(b) };
}

int main(int argc, char** argv)
{
    std::string filename;
    if (argc > 1)
    {
        filename = argv[1];
        if (!filename.ends_with(".java"))
        {
            fmt::print("{} is not a valid .java file!\n", filename);
            return 0;
        }
    }
    else
    {
        for (auto const& dir_entry : std::filesystem::directory_iterator{"."})
        {
            if (dir_entry.is_regular_file())
            {
                auto filename_entry = dir_entry.path().filename().string();
                if (filename_entry.ends_with(".java"))
                {
                    if (!filename.empty())
                    {
                        fmt::print("More than 1 .java file detected. Aborting.\n");
                        return 0;
                    }
                    filename = filename_entry;
                }
            }
        }
    }

    if (filename.empty())
    {
        fmt::print("No .java file detected. Aborting.\n");
        return 0;
    }

    int r = system(fmt::format("javac {}", filename).data());

    if (r != 0)
    {
        fmt::print("The .java has errors. Aborting.\n");
        return 0;
    }

    std::string project_name = filename.substr(0, filename.find('.'));

    std::ifstream file(project_name + ".class", std::ios::binary);

    file.seekg(0, std::ios::end);
    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    assert(fileSize > 0);

    Buffer buffer(fileSize);
    file.read((char*) &buffer[0], fileSize);

    auto magic = r32();
    assert(magic == 0xCAFEBABE);

    [[maybe_unused]] auto minor = r16();
    [[maybe_unused]] auto major = r16();

    constantPool.push_back({});
    auto constant_pool_count = r16();
    for (int i = 0; i < constant_pool_count - 1; ++i)
    {
        auto tag = r8();
        switch (tag)
        {
        case CONSTANT_Utf8:
        {
            auto length = r16();
            Utf8 info;
            info.length = length;

            for (int i = 0; i < length; ++i)
            {
                info.bytes.push_back(r8());
            }

            constantPool.push_back(info);
            break;
        }
        case CONSTANT_Integer:
            assert(false);
            break;
        case CONSTANT_Float:
        {
            Float info { r32() };
            constantPool.push_back(info);
            break;
        }
        case CONSTANT_Long:
            assert(false);
            break;
        case CONSTANT_Double:
            assert(false);
            break;
        case CONSTANT_Class:
        {
            Class info { r16() };
            constantPool.push_back(info);
            break;
        }
        case CONSTANT_String:
        {
            String info { r16() };
            constantPool.push_back(info);
            break;
        }
        case CONSTANT_Fieldref:
        {
            Fieldref info { r16(), r16() };
            constantPool.push_back(info);
            break;
        }
        case CONSTANT_Methodref:
        {
            Methodref info { r16(), r16() };
            constantPool.push_back(info);
            break;
        }
        case CONSTANT_InterfaceMethodref:
        {
            InterfaceMethodref info { r16(), r16() };
            constantPool.push_back(info);
            break;
        }
        case CONSTANT_NameAndType:
        {
            NameAndType info { r16(), r16() };
            constantPool.push_back(info);
            break;
        }
        case CONSTANT_MethodHandle:
        {
            MethodHandle info { r8(), r16() };
            constantPool.push_back(info);
            break;
        }
        case CONSTANT_MethodType:
        {
            r16();
            constantPool.push_back({});
            break;
        }
        case CONSTANT_InvokeDynamic:
        {
            InvokeDynamic info { r16(), r16() };
            constantPool.push_back(info);
            break;
        }
        default:
            assert(false);
        }
    }

    [[maybe_unused]] auto access_flags = r16();
    [[maybe_unused]] auto this_class = r16();
    [[maybe_unused]] auto super_class = r16();
    auto interfaces_count = r16();
    assert(interfaces_count == 0);

    auto fields_count = r16();
    for (u2 i = 0; i < fields_count; ++i)
    {
        auto access_flags = r16();
        auto name_index = r16();
        auto descriptor_index = r16();
        auto attributes_count = r16();
        assert(attributes_count == 0);

        if ((access_flags & 0x0008) > 0)
        {
            auto name = getStringFromUtf8(name_index);
            auto descriptor = getStringFromUtf8(descriptor_index);

            fields.push_back({ name, getTypeFromDescriptor(descriptor) });
        }
    }

    struct MethData
    {
        std::string name;
        std::string descriptor;
        Buffer buffer;
    };

    std::vector<MethData> methodsToDecompile;

    auto methods_count = r16();
    for (int i = 0; i < methods_count; ++i)
    {
        auto access_flags = r16();
        auto name_index = r16();
        auto descriptor_index = r16();
        auto attributes_count = r16();
        std::vector<attribute_info> attributes;

        for (u2 i = 0; i < attributes_count; ++i)
        {
            attributes.push_back(readAttribute(buffer));
        }

        if (access_flags & 0x0008)
        {
            auto name = getStringFromUtf8(name_index);
            auto descriptor = getStringFromUtf8(descriptor_index);

            for (auto & attr : attributes)
            {
                auto attribute_name = getStringFromUtf8(attr.attribute_name_index);

                if (attribute_name == "Code")
                {
                    methodsToDecompile.push_back({ name, descriptor, attr.info });
                }
                else if (attribute_name == "LineNumberTable")
                {
                }
                else
                {
                    assert(false);
                }
            }
        }
    }

    std::string board_name = "pico";

    auto attributes_count = r16();
    std::vector<attribute_info> attributes;

    for (u2 i = 0; i < attributes_count; ++i)
    {
        attributes.push_back(readAttribute(buffer));
        auto attribute_name = getStringFromUtf8(attributes.back().attribute_name_index);
        if (attribute_name == "RuntimeInvisibleAnnotations")
        {
            auto buffer = attributes.back().info;

            auto num_annotations = r16();
            for (u2 ii = 0; ii < num_annotations; ++ii)
            {
                auto type_index = r16();
                auto type_name = getStringFromUtf8(type_index);
                auto num_element_value_pairs = r16();
                for (u2 iii = 0; iii < num_element_value_pairs; ++iii)
                {
                    auto element_name_index = r16();
                    auto element_name = getStringFromUtf8(element_name_index);

                    // value: element_value
                    auto tag = r8();
                    assert(tag == 'e');

                    if (type_name == "Lpico/board;" && element_name == "value")
                    {
                        auto type_name_index = r16();
                        auto type_name = getStringFromUtf8(type_name_index);
                        auto const_name_index = r16();
                        auto const_name = getStringFromUtf8(const_name_index);

                        if (type_name == "Lpico/BoardType;")
                        {
                            board_name = const_name;
                        }
                        else
                        {
                            assert(false);
                        }

                        iii = num_element_value_pairs;
                        ii = num_annotations;
                    }
                }
            }
        }
        else if (attribute_name == "SourceFile")
        {
        }
        else if (attribute_name == "BootstrapMethods")
        {
            auto buffer = attributes.back().info;

            u2 num_bootstrap_methods = r16();

            for (u2 ii = 0; ii < num_bootstrap_methods; ++ii)
            {
                u2 bootstrap_method_ref = r16();
                auto bootstrap_method_handle = std::get<MethodHandle>(constantPool[bootstrap_method_ref]);
                auto bootstrap_method = std::get<Methodref>(constantPool[bootstrap_method_handle.reference_index]);
                auto bootstrap_className = getStringFromUtf8(std::get<Class>(constantPool[bootstrap_method.class_index]).name_index);
                auto bootstrap_descriptor = getStringFromUtf8(std::get<NameAndType>(constantPool[bootstrap_method.name_and_type_index]).descriptor_index);
                auto bootstrap_methodName = getStringFromUtf8(std::get<NameAndType>(constantPool[bootstrap_method.name_and_type_index]).name_index);

                std::vector<std::string> args;
                u2 num_bootstrap_arguments = r16();
                for (u2 arg = 0; arg < num_bootstrap_arguments; ++arg)
                {
                    u2 bootstrap_argument = r16();
                    auto bootstrap_pool = constantPool[bootstrap_argument];
                    if (std::holds_alternative<MethodHandle>(bootstrap_pool))
                    {
                        auto handle = std::get<MethodHandle>(bootstrap_pool);
                        assert(handle.reference_kind == REF_invokeStatic);

                        auto method = std::get<Methodref>(constantPool[handle.reference_index]);
                        auto className = getStringFromUtf8(std::get<Class>(constantPool[method.class_index]).name_index);
                        auto descriptor = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).descriptor_index);
                        auto methodName = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).name_index);

                        std::string caster;
                        if (descriptor == "(II)V")
                        {
                            caster = "(gpio_irq_callback_t)";
                        }

                        auto fullName = fmt::format("{}{}::{}", caster, className, methodName);
                        boost::replace_all(fullName, "/"s, "::"s);

                        args.push_back(fullName);
                    }
                    else if (std::holds_alternative<String>(bootstrap_pool))
                    {
                        auto data = std::get<String>(bootstrap_pool);
                        auto str = getStringFromUtf8(data.string_index);
                        args.push_back(str);
                    }
                    else
                    {
                        assert(false);
                    }
                    {

                    }
                }

                if (bootstrap_methodName == "makeConcatWithConstants")
                {
                    callbacksMethods.push_back(args[0]);
                }
            }
        }
        else if (attribute_name == "InnerClasses")
        {
        }
        else
        {
            assert(false);
        }
    }

    for (auto & meth : methodsToDecompile)
    {
        auto name = meth.name;
        auto descriptor = meth.descriptor;
        auto buffer = meth.buffer;

        [[maybe_unused]] u2 max_stack = r16();
        [[maybe_unused]] u2 max_locals = r16();
        u4 code_length = r32();

        Buffer code;
        for (u4 i = 0; i < code_length; ++i)
        {
            code.push_back(r8());
        }

        u2 exception_table_length = r16();
        for (u4 i = 0; i < exception_table_length; ++i)
        {
            [[maybe_unused]] u2 start_pc = r16();
            [[maybe_unused]] u2 end_pc = r16();
            [[maybe_unused]] u2 handler_pc = r16();
            [[maybe_unused]] u2 catch_type = r16();
        };

        std::vector<std::tuple<u2, u2>> lineNumbers;

        u2 attributes_count = r16();
        for (u2 i = 0; i < attributes_count; ++i)
        {
            attributes.push_back(readAttribute(buffer));
            auto attribute_name = getStringFromUtf8(attributes.back().attribute_name_index);
            if (attribute_name == "LineNumberTable")
            {
                auto buffer = attributes.back().info;
                u2 line_number_table_length = r16();
                for (u2 a = 0; a < line_number_table_length; ++a)
                {
                    u2 start_pc = r16();
                    u2 line_number = r16();

                    lineNumbers.emplace_back(start_pc, line_number);
                }
            }
            else if (attribute_name == "StackMapTable")
            {
            }
            else
            {
                assert(false);
            }
        }

        if (name == "<clinit>")
        {
            name = "static_init";
        }

        FunctionData funData;
        funData.name = name;
        funData.descriptor = descriptor;
        funData.instructions = lineAnalyser(code, name, lineNumbers);

        functions.push_back(funData);
    }

    build_pico(project_name, getBoardTypeFromString(board_name));

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

attribute_info readAttribute(Buffer & buffer)
{
    attribute_info info;

    info.attribute_name_index = r16();
    auto length = r32();
    info.attribute_length = length;

    for (u4 i = 0; i < length; ++i)
    {
        info.info.push_back(r8());
    }

    return info;
}

std::string getTypeFromDescriptor(std::string descriptor)
{
    if (descriptor == "I")
    {
        return "int";
    }

    assert(false);
    return {};
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

    assert(false);
}

Board getBoardTypeFromString(std::string board_name)
{
    std::transform(begin(board_name), end(board_name), begin(board_name), [](auto c) { return std::tolower(c); });

    if (board_name == "pico")         return Board::Pico;
    if (board_name == "picow")        return Board::PicoW;
    if (board_name == "tiny2040")     return Board::Tiny2040;
    if (board_name == "tiny2040_2mb") return Board::Tiny2040_2mb;
    if (board_name == "badger2040")   return Board::Badger2040;

    assert(false);
}

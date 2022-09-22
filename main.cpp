#include "globals.h"
#include "lineanalyser.h"

u4 countArgs(std::string str)
{
    auto start = str.find('(');
    auto end = str.find(')');
    auto d = end - start - 1;

    return d;
}

attribute_info readAttribute(Buffer & buffer);

std::vector<Instruction> convertBytecode(Buffer & buffer, std::string function, u2 depth);
void initializeFields(Buffer & buffer);
std::string getTypeFromDescriptor(std::string descriptor);

ConstantPool constantPool;
std::vector<FieldData> fields;
std::vector<FunctionData> functions;

std::string getStringFromUtf8(int index)
{
    auto b = std::get<Utf8>(constantPool[index]).bytes;
    return std::string { begin(b), end(b) };
}

int main(int argc, char** argv)
{
    std::string filename = "Tiny2040blinky.class";
    if (argc > 1)
    {
        filename = argv[1];
    }
    fmt::print("project: {}\n", filename);

    std::string project_name = filename.substr(0, filename.find('.'));

    std::ifstream file(filename, std::ios::binary);

    file.seekg(0, std::ios::end);
    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

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
            assert(false);
            break;
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
            assert(false);
            break;
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
            assert(false);
            break;
        case CONSTANT_MethodType:
            assert(false);
            break;
        case CONSTANT_InvokeDynamic:
            assert(false);
            break;
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
                    auto buffer = attr.info;
                    [[maybe_unused]] u2 max_stack = r16();
                    u2 max_locals = r16();
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

                    std::vector<u2> lineNumbers;

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
                                [[maybe_unused]] u2 line_number = r16();

                                lineNumbers.push_back(start_pc);
                            }
                        }
                    }

                    if (name == "<clinit>")
                    {
                        //initializeFields(code);
                        name = "static_init";
                    }
                    //else
                    {
                        FunctionData funData;
                        funData.name = name;
                        funData.descriptor = descriptor;
                        funData.instructions = lineAnalyser(code, name, lineNumbers);

                        if (name == "main")
                        {
                            Instruction inst;
                            inst.position = 999999;
                            inst.opcode = fmt::format("static_init();");
                            funData.instructions.insert(funData.instructions.begin(), inst);
                        }

                        if (false && max_locals)
                        {
                            Instruction inst;
                            inst.position = 999999;
                            inst.opcode = fmt::format("int locals[{}];", max_locals);
                            funData.instructions.insert(funData.instructions.begin(), inst);
                        }

                        functions.push_back(funData);
                    }
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
                        i = attributes_count;
                    }
                }
            }
        }
    }

    build_pico(project_name, board_name);

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

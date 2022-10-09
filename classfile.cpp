#include "classfile.h"
#include "boards/gamebuino.h"
#include "boost/algorithm/string.hpp"
#include <fstream>

enum
{
    T_NONE    = 0,
    T_STRING  = 1,
    T_OBJECT  = 2,
    T_ARRAY   = 3,
    T_BOOLEAN = 4,
    T_CHAR    = 5,
    T_FLOAT   = 6,
    T_DOUBLE  = 7,
    T_BYTE    = 8,
    T_SHORT   = 9,
    T_INT     = 10,
    T_LONG    = 11,
};

std::string getType(int type)
{
    switch (type)
    {
    case T_BOOLEAN: return "bool";
    case T_CHAR:    return "char";
    case T_FLOAT:   return "float";
    case T_DOUBLE:  return "double";
    case T_BYTE:    return "int8_t";
    case T_SHORT:   return "int16_t";
    case T_INT:     return "int32_t";
    case T_LONG:    return "int64_t";
    }

    throw fmt::format("Invalid type: '{}'.", type);
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

std::string getTypeFromDescriptor(std::string descriptor, u8 flags)
{
    int count = 0;
    while (descriptor.size() && descriptor.front() == '[')
    {
        descriptor.erase(descriptor.begin());
        ++count;
    }

    std::string prefix;
    if (flags & CONST_TYPE) prefix += "const ";
    if (flags & UNSIGNED_TYPE) prefix += "u";

    if (descriptor == "I")
    {
        return prefix + "int32_t";
    }

    if (descriptor == "B")
    {
        return prefix + "int8_t";
    }

    if (descriptor == "S")
    {
        return prefix + "int16_t";
    }

    if (descriptor == "Z")
    {
        return "bool";
    }

    if (descriptor.starts_with("L") && descriptor.ends_with(";"))
    {
        auto jt = descriptor.substr(1, descriptor.size() - 2);
        if (jt.starts_with("types/"))
        {
            return jt.substr(6) + "_t";
        }
        return javaToCpp(jt);
    }

    throw fmt::format("Invalid type used as a static field: '{}'.", descriptor);
}

template<class> inline constexpr bool always_false_v = false;

std::string getAsString(const Value & value)
{
    return std::visit([](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, int32_t>)
        {
            return fmt::format("{}", arg);
        }
        else if constexpr (std::is_same_v<T, int64_t>)
        {
            return fmt::format("{}L", arg);
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            return fmt::format("{}", arg);
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            return fmt::format("{}", arg);
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            return arg;
        }
        else if constexpr (std::is_same_v<T, Array>)
        {
            std::string output;
            for (size_t i = 0; i < arg.populate.size(); ++i)
            {
                if (i > 0)
                {
                    output += ", ";
                }
                output += arg.populate[i];
            }

            return fmt::format("{{ {} }}", output);
        }
        else if constexpr (std::is_same_v<T, Object>)
        {
            return arg.type;
        }
        else
        {
            static_assert(always_false_v<T>, "non-exhaustive visitor!");
        }
    }, value);
}

std::string invertBinaryOperator(std::string binop)
{
    if (binop == "!=") return "==";
    if (binop == "==") return "!=";
    if (binop == ">=") return  "<";
    if (binop ==  "<") return ">=";
    if (binop == "<=") return ">";
    if (binop ==  ">") return  "<=";

    throw fmt::format("'{}' is not a valid binary operator.", binop);
}

ClassFile::ClassFile(std::string filename, std::string projectName, bool partial)
{
    project_name = projectName;

    fileName = filename.substr(0, filename.find('.'));
    std::ifstream file(fileName + ".class", std::ios::binary);

    file.seekg(0, std::ios::end);
    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize == 0)
    {
        throw fmt::format("File is empty.");
    }

    Buffer buffer(fileSize);
    file.read((char*) &buffer[0], fileSize);

    auto magic = r32();
    if (magic != 0xCAFEBABE)
    {
        throw fmt::format("Invalid class file. Wrong magic number.");
    }

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
        {
            auto v = s32();
            constantPool.push_back(v);
            break;
        }
        case CONSTANT_Float:
        {
            auto v = r32();
            auto info = std::bit_cast<float>(v);
            constantPool.push_back(info);
            break;
        }
        case CONSTANT_Long:
        {
            auto h = r32();
            auto l = r32();
            s8 info = (s8{h} << 32) | l;
            constantPool.push_back(info);
            ++i;
            constantPool.push_back({});
            break;
        }
        case CONSTANT_Double:
        {
            auto h = r32();
            auto l = r32();
            u8 v = (s8{h} << 32) | l;
            auto info = std::bit_cast<double>(v);
            constantPool.push_back(info);
            ++i;
            constantPool.push_back({});
            break;
        }
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
            MethodType info { r16() };
            constantPool.push_back(info);
            break;
        }
        case CONSTANT_InvokeDynamic:
        {
            InvokeDynamic info { r16(), r16() };
            constantPool.push_back(info);
            break;
        }
        default:
            throw fmt::format("Unknown constant type: '{}'.", tag);
        }
    }

    [[maybe_unused]] auto access_flags = r16();
    [[maybe_unused]] auto this_class = r16();
    [[maybe_unused]] auto super_class = r16(); // TODO: check if super_class is object
    auto interfaces_count = r16();
    if (interfaces_count != 0)
    {
        throw fmt::format("The class must not have any interfaces.");
    }

    auto fields_count = r16();
    for (u2 i = 0; i < fields_count; ++i)
    {
        auto access_flags = r16();
        auto name_index = r16();
        auto descriptor_index = r16();
        auto attributes_count = r16();

        u8 flags = 0;

        if (access_flags & ACC_FINAL)
        {
            flags += CONST_TYPE;
        }

        for (u2 i = 0; i < attributes_count; ++i)
        {
            auto attr = readAttribute(buffer);
            auto attribute_name = getStringFromUtf8(attr.attribute_name_index);
            if (attribute_name == "RuntimeInvisibleAnnotations")
            {
                auto buffer = attr.info;

                auto num_annotations = r16();
                for (u2 ii = 0; ii < num_annotations; ++ii)
                {
                    auto type_index = r16();
                    auto type_name = getStringFromUtf8(type_index);
                    [[maybe_unused]] auto num_element_value_pairs = r16();

                    if (type_name == "Ltypes/unsigned;")
                    {
                        flags += UNSIGNED_TYPE;
                    }
                }
            }
        }

        auto name = getStringFromUtf8(name_index);
        auto descriptor = getStringFromUtf8(descriptor_index);

        fields.push_back({ name, getTypeFromDescriptor(descriptor, flags), descriptor[0] == '[', access_flags });
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
        [[maybe_unused]] auto access_flags = r16();
        auto name_index = r16();
        auto descriptor_index = r16();
        auto attributes_count = r16();
        std::vector<attribute_info> attributes;

        for (u2 i = 0; i < attributes_count; ++i)
        {
            attributes.push_back(readAttribute(buffer));
        }

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
                throw fmt::format("Unhandled method attribute: '{}'.", attribute_name);
            }
        }
    }

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

                    if (type_name == "Lboard/Board;" && element_name == "value")
                    {
                        if (tag != 'e')
                        {
                            throw fmt::format("Annotation '@Board' must contains an enumeration.");
                        }

                        auto type_name_index = r16();
                        auto type_name = getStringFromUtf8(type_name_index);
                        auto const_name_index = r16();
                        auto const_name = getStringFromUtf8(const_name_index);

                        if (type_name == "Lboard/Type;")
                        {
                            board_name = const_name;
                        }
                        else
                        {
                            throw fmt::format("Annotation '@Board' must contains a board.Type value.");
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
                        if (handle.reference_kind != REF_invokeStatic)
                        {
                            throw fmt::format("Only static references are valid as a bootstrap method handle parameter.");
                        }

                        auto method = std::get<Methodref>(constantPool[handle.reference_index]);
                        auto className = getStringFromUtf8(std::get<Class>(constantPool[method.class_index]).name_index);
                        auto descriptor = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).descriptor_index);
                        auto methodName = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).name_index);

                        auto fullName = fmt::format("{}::{}", className, methodName);

                        std::string caster;
                        if (fullName == "ButtonBlinky::gpio_irq_callback")
                        {
                            caster = "(gpio_irq_callback_t)";
                        }

                        fullName = fmt::format("{}{}", caster, fullName);
                        fullName = javaToCpp(fullName);

                        args.push_back(fullName);
                    }
                    else if (std::holds_alternative<String>(bootstrap_pool))
                    {
                        auto data = std::get<String>(bootstrap_pool);
                        auto str = getStringFromUtf8(data.string_index);
                        args.push_back(str);
                    }
                    else if (std::holds_alternative<MethodType>(bootstrap_pool))
                    {
                        args.push_back("");
                    }
                    else
                    {
                        throw fmt::format("Unhandled parameter type for bootstrap method '{}' at index '{}'", bootstrap_methodName, arg);
                    }
                }

                if (bootstrap_methodName == "makeConcatWithConstants")
                {
                    callbacksMethods.push_back(args[0]);
                }
                else if (bootstrap_methodName == "metafactory")
                {
                    callbacksMethods.push_back(args[1]);
                }
                else
                {
                    throw fmt::format("Unhandled bootstrap method: '{}'.", bootstrap_methodName);
                }
            }
        }
        else if (attribute_name == "InnerClasses")
        {
        }
        else
        {
            throw fmt::format("Unhandled class attribute: '{}'.", attribute_name);
        }
    }

    if (partial)
    {
        return;
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
                throw fmt::format("Unhandled method attribute: '{}'.", attribute_name);
            }
        }

        if (name == STATIC_INIT)
        {
            lineAnalyser(code, STATIC_INIT, lineNumbers);
        }
        else
        {
            auto skip = hasBoard() && name == CONSTRUCTOR;
            if (!skip)
            {
                FunctionData funData;
                funData.name = name;
                funData.descriptor = descriptor;
                funData.instructions = lineAnalyser(code, name, lineNumbers);

                functions.push_back(funData);
            }
        }
    }
}

bool ClassFile::hasBoard() const
{
    return board_name.size() > 0;
}

std::string ClassFile::boardName() const
{
    return board_name;
}

void ClassFile::generate(const std::vector<ClassFile> & files, Board board)
{
    std::ofstream output_h(fileName + ".h");

    output_h << "#ifndef " << boost::to_upper_copy(fileName) << "_H\n"
             << "#define " << boost::to_upper_copy(fileName) << "_H\n";

    if (board != Board::Gamebuino)
    {
        output_h << "#include \"pico-java.h\"\n"
                 << "#if __has_include(\"" << RESOURCES_FILE << ".h\")\n"
                 << "#include \"" << RESOURCES_FILE << ".h\"\n"
                 << "#endif\n"
                 << "#if __has_include(\"" << USER_FILE << ".h\")\n"
                 << "#include \"" << USER_FILE << ".h\"\n"
                 << "#endif\n";

        for (auto & f : files)
        {
            if (f.fileName != fileName)
                output_h << "#include \"" << f.fileName << ".h\"\n";
        }

        output_h << '\n';
    }

    if (!hasBoard())
    {
        output_h << "class " << fileName << " {\n"
                 << "public:\n";
    }

    if (fields.size())
    {
        output_h << '\n';

        for (auto & field : fields)
        {
            if (hasBoard())
            {
                if ((field.flags & ACC_PUBLIC) == 0) continue;

                output_h << "extern ";
            }

            output_h << field.type;
            if (!field.init.has_value() && field.isArray)
            {
                output_h << '*';
            }
            output_h << " " << field.name;

            if (field.init.has_value())
            {
                if (field.isArray)
                {
                    output_h << "[]";
                }

                if (!hasBoard() && field.init.value() != "null")
                {
                    output_h << " = " << field.init.value();
                }
            }

            output_h << ";\n";
        }
    }

    for (auto & func : functions)
    {
        if (!hasBoard() || (func.flags & ACC_PUBLIC))
        {
            output_h << '\n';

            if (func.name == CONSTRUCTOR)
            {
                output_h << fileName;
            }
            else
            {
                if ((func.flags & ACC_STATIC))
                {
                    output_h << "static ";
                }
                output_h << getReturnType(func.descriptor) << " " << func.name;
            }
            output_h << "(" << generateParameters(func.descriptor, true) << ");\n";
        }
    }

    if (!hasBoard())
    {
        output_h << "};\n";
    }

    output_h << "#endif\n";

    output_h.close();

    std::string extension;
    switch (board)
    {
    case Board::Gamebuino:
        extension = "ino";
        break;
    default:
        extension = "cpp";
        break;
    }

    std::ofstream output_c(fileName + "." + extension);

    if (hasBoard() || board != Board::Gamebuino)
    {
        switch (board)
        {
        case Board::Gamebuino:
            output_c << "#include \"gamebuino-java.h\"\n";
            break;
        default: // pico boards
            output_c << "#include \"pico-java.h\"\n";
            break;
        }

        output_c << "#if __has_include(\"" << RESOURCES_FILE << ".h\")\n"
                 << "#include \"" << RESOURCES_FILE << ".h\"\n"
                 << "#endif\n"
                 << "#if __has_include(\"" << USER_FILE << ".h\")\n"
                 << "#include \"" << USER_FILE << ".h\"\n"
                 << "#endif\n";
        for (auto & f : files)
        {
            if (!f.hasBoard())
            {
                output_c << "#include \"" << f.fileName << ".h\"\n";
            }
        }
    }

    if (hasBoard())
    {
        output_c << '\n';

        for (auto & field : fields)
        {
            if ((field.flags & ACC_PUBLIC)) continue;

            if (field.init.has_value() && field.init.value() == "null")
            {
                output_c << "extern ";
            }

            output_c << field.type;
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

                if (field.init.value() != "null")
                {
                    output_c << " = " << field.init.value();
                }
            }

            output_c << ";\n";
        }
    }

    for (auto & func : functions)
    {
        output_c << '\n';

        std::string classNameSpace;

        if (!hasBoard())
        {
            classNameSpace = fileName + "::";
        }

        if (func.name == CONSTRUCTOR)
        {
            output_c << classNameSpace << fileName;
        }
        else
        {
            output_c << getReturnType(func.descriptor) << " " << classNameSpace << func.name;
        }
        output_c << "(" << generateParameters(func.descriptor, !hasBoard()) << ")\n{\n";

        int depth = 0;
        for (auto & inst : func.instructions)
        {
            if (inst.opcode.starts_with("}")) --depth;
            if (inst.opcode.size())
            {
                output_c << std::string(depth + 1, '\t') << inst.opcode << '\n';
            }
            if (inst.opcode.starts_with("{")) ++depth;
        }
        output_c << "}\n";
    }

    output_c.close();
}

std::vector<Instruction> ClassFile::lineAnalyser(Buffer & buffer, const std::string & name, std::vector<std::tuple<u2, u2>> lineNumbers)
{
    insts.clear();
    localsTypes.clear();
    localsTypes.push_back({});
    closingBraces.clear();

    fullBuffer = &buffer;
    lines = &lineNumbers;

    std::unordered_map<int, Buffer> buffers;
    std::set<int> order;
    for (size_t i = 0; i < lineNumbers.size(); ++i)
    {
        auto begin = std::get<0>(lineNumbers[i]);
        auto end = (i + 1 < lineNumbers.size() ? std::get<0>(lineNumbers[i + 1]) : buffer.size());
        auto sub = Buffer(&buffer[begin], &buffer[end]);

        auto line = std::get<1>(lineNumbers[i]);
        buffers[line].insert(
                    buffers[line].end(),
                    std::make_move_iterator(sub.begin()),
                    std::make_move_iterator(sub.end())
                    );
        order.insert(line);
    }

    for (auto key : order)
    {
        auto ret = decodeBytecodeLine(buffers[key], name, key);
        insts.insert(
                    insts.end(),
                    std::make_move_iterator(ret.begin()),
                    std::make_move_iterator(ret.end())
                    );
    }

    if (closingBrackets.size() != 0)
    {
        throw fmt::format("Mismatched brackets.");
    }

    return insts;
}

std::vector<Instruction> ClassFile::decodeBytecodeLine(Buffer & buffer, const std::string & name, u4 position)
{
    auto buffer_size = buffer.size();
    auto start_pc = getOpcodeFromLine(position);

    std::vector<Operation> operations;
    std::vector<Instruction> lineInsts = { Instruction() };
    Instruction & inst = lineInsts.front();
    inst.position = position;

    while (buffer.size())
    {
        auto opcode = r8();

        switch (opcode)
        {
        case bipush:
        {
            auto value = r8();
            stack.push_back(value);
            break;
        }
        case anewarray:
        {
            auto val = stack.back();
            stack.pop_back();
            auto size = std::get<int>(val);
            auto type = r16();
            auto className = getStringFromUtf8(std::get<Class>(constantPool[type]).name_index);

            std::string cppType;
            if (className == "java/lang/String")
            {
                cppType = "std::string";
            }
            else if (className == "java/lang/Object")
            {
                cppType = "Object";
            }
            else
            {
                throw fmt::format("Only String is handled as an array type.");
            }

            Array arr;
            arr.size = size;
            arr.type = cppType;
            arr.position = start_pc + buffer_size - buffer.size() - 1;
            stack.push_back(arr);
            break;
        }
        case astore:
        case astore_0:
        case astore_1:
        case astore_2:
        case astore_3:
        {
            int index;
            if (opcode == astore)
            {
                index = r8();
            }
            else
            {
                index = opcode - astore_0;
            }

            auto v = stack.back();
            stack.pop_back();
            auto & locals = localsTypes.back();
            auto localType = findLocal(index);

            Operation op;
            op.type = OpType::Store;
            op.store.index = index;

            if (std::holds_alternative<Array>(v))
            {
                auto arr = std::get<Array>(v);

                op.store.size = arr.size;
                op.store.position = arr.position;
                op.store.arr_type = arr.type;
                op.store.populate = arr.populate;

                if (localType != T_ARRAY)
                {
                    op.store.type = arr.type;
                    locals[index] = T_ARRAY;
                }
            }
            else if (std::holds_alternative<std::string>(v))
            {
                auto str = std::get<std::string>(v);
                op.store.arr_type = "std::string";
                op.store.value = str;
                if (localType != T_STRING)
                {
                    op.store.type =  op.store.arr_type;
                    locals[index] = T_STRING;
                }
            }
            else if (std::holds_alternative<Object>(v))
            {
                auto obj = std::get<Object>(v);
                op.store.arr_type = obj.type;
                if (localType != T_OBJECT)
                {
                    op.store.type =  op.store.arr_type;
                    op.store.value = obj.ctor;
                    locals[index] = T_OBJECT;
                }
            }
            else
            {
                assert(false);
            }

            operations.push_back(op);
            break;
        }
        case iconst_0:
        case iconst_1:
        case iconst_2:
        case iconst_3:
        case iconst_4:
        case iconst_5:
        {
            stack.push_back(opcode - iconst_0);
            break;
        }
        case fconst_0:
        case fconst_1:
        case fconst_2:
        {
            stack.push_back(static_cast<float>(opcode - fconst_0));
            break;
        }
        case istore:
        case lstore:
        case fstore:
        case dstore:
        case istore_0:
        case istore_1:
        case istore_2:
        case istore_3:
        {
            int unnumbered;
            int zero_numbered;
            int this_type;
            switch (opcode)
            {
            case istore:
            case istore_0:
            case istore_1:
            case istore_2:
            case istore_3:
                unnumbered = istore;
                zero_numbered = istore_0;
                this_type = T_INT;
                break;
            case lstore:
                unnumbered = lstore;
                zero_numbered = lstore_0;
                this_type = T_LONG;
                break;
            case fstore:
                unnumbered = fstore;
                zero_numbered = fstore_0;
                this_type = T_FLOAT;
                break;
            case dstore:
                unnumbered = dstore;
                zero_numbered = dstore_0;
                this_type = T_DOUBLE;
                break;
            }

            auto v = stack.back();
            stack.pop_back();

            int index;
            if (opcode == unnumbered)
            {
                index = r8();
            }
            else
            {
                index = opcode - zero_numbered;
            }

            Operation op;
            op.type = OpType::Store;
            op.store.index = index;

            auto & locals = localsTypes.back();
            auto localType = findLocal(index);
            if (localType != this_type)
            {
                op.store.type = getType(this_type);
                locals[index] = this_type;
            }

            op.store.value = getAsString(v);

            operations.push_back(op);
            break;
        }
        case iload:
        case iload_0:
        case iload_1:
        case iload_2:
        case iload_3:
        {
            int index;
            if (opcode == iload)
            {
                index = r8();
            }
            else
            {
                index = opcode - iload_0;
            }

            stack.push_back(fmt::format("local_{}", index));
            break;
        }
        case aload:
        case aload_0:
        case aload_1:
        case aload_2:
        case aload_3:
        {
            int index;
            if (opcode == aload)
            {
                index = r8();
            }
            else
            {
                index = opcode - aload_0;
            }

            stack.push_back(fmt::format("local_{}", index));
            break;
        }
        case arraylength:
        {
            auto val = stack.back();
            stack.pop_back();
            stack.push_back(fmt::format("{}.size()", std::get<std::string>(val)));
            break;
        }
        case if_icmpeq:
        case if_icmpne:
        case if_icmplt:
        case if_icmpge:
        case if_icmpgt:
        case if_icmple:
        case if_acmpeq:
        case if_acmpne:
        {
            auto correction = buffer_size - buffer.size() - 1;
            auto offset = s16();
            auto absolute = start_pc + correction + offset;

            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            std::string binop;
            switch (opcode)
            {
            case if_icmpeq: binop = "!="; break;
            case if_icmpne: binop = "=="; break;
            case if_icmplt: binop = ">="; break;
            case if_icmpge: binop =  "<"; break;
            case if_icmpgt: binop = "<="; break;
            case if_icmple: binop =  ">"; break;
            case if_acmpeq: binop = "!="; break;
            case if_acmpne: binop = "=="; break;
            default: assert(false);
            }

            Operation op;
            op.type = OpType::Cond;
            op.cond.op = binop;
            op.cond.left = getAsString(left);
            op.cond.right = getAsString(right);
            op.cond.absolute = absolute;

            operations.push_back(op);

            if ((*fullBuffer)[absolute - 3] == goto_)
            {
                skippedGotos.insert(absolute - 3);
            }
            break;
        }
        case iinc:
        {
            Operation op;
            op.type = OpType::Inc;
            op.inc.index = r8();
            op.inc.constant = r8();
            operations.push_back(op);
            break;
        }
        case goto_:
        {
            auto correction = buffer_size - buffer.size() - 1;
            auto offset = s16();
            auto absolute = start_pc + correction + offset;

            if (skippedGotos.contains(start_pc + correction))
            {
                break;
            }

            Operation op;
            op.type = OpType::Jump;
            op.jump.absolute = absolute;
            operations.push_back(op);
            break;
        }
        case invokedynamic:
        {
            auto id = r16();
            auto zero = r16();
            if (zero != 0)
            {
                throw fmt::format("zero is not '0' but '{}'.", zero);
            }

            auto invokeDyn = std::get<InvokeDynamic>(constantPool[id]);
            auto tpl = callbacksMethods[invokeDyn.bootstrap_method_attr_index];
            if (tpl.contains(0x02))
            {
                throw fmt::format("Unhandled 0x02 template.");
            }
            if (tpl.contains(0x01))
            {
                std::string newTpl = "\"";

                for (auto c : tpl)
                {
                    if (c == 0x01)
                    {
                        auto var = stack.back();
                        stack.pop_back();
                        newTpl += fmt::format("\" + {} + \"", getAsString(var));
                    }
                    else
                    {
                        newTpl += c;
                    }
                }
                newTpl += "\"";

                if (newTpl.starts_with("\"\" + "))
                {
                    newTpl = newTpl.substr(5);
                }

                if (newTpl.ends_with(" + \"\""))
                {
                    newTpl = newTpl.substr(0, newTpl.size() - 5);
                }
                tpl = newTpl;
            }
            stack.push_back(tpl);
            break;
        }
        case iastore:
        case aastore:
        case fastore:
        case lastore:
        case dastore:
        case bastore:
        {
            auto value = stack.back();
            stack.pop_back();
            auto index = stack.back();
            stack.pop_back();
            auto arr = stack.back();
            stack.pop_back();

            if (std::holds_alternative<std::string>(arr))
            {
                Operation op;
                op.type = OpType::IndexedStore;
                op.istore.index = getAsString(index);
                op.istore.array = getAsString(arr);
                op.istore.value = getAsString(value);
                operations.push_back(op);
            }
            else if (std::holds_alternative<Array>(arr))
            {
                auto orig = stack.back();
                stack.pop_back();
                std::get<Array>(orig).populate.push_back(getAsString(value));
                stack.push_back(orig);
            }
            else
            {
                throw fmt::format("Unhandled type (id: {}). Expecting 'Array' or 'string'.", value.index());
            }
            break;
        }
        case return_:
        {
            Operation op;
            op.type = OpType::Return;
            if (name == "main")
            {
                op.ret.value = "0";
            }
            operations.push_back(op);
            break;
        }
        case ireturn:
        case lreturn:
        case freturn:
        case dreturn:
        case areturn:
        {
            auto val = stack.back();
            stack.pop_back();

            Operation op;
            op.type = OpType::Return;
            op.ret.value = getAsString(val);
            operations.push_back(op);
            break;
        }
        case imul:
        case lmul:
        case fmul_:
        case dmul:
        {
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("({} * {})", getAsString(left), getAsString(right)));
            break;
        }
        case iadd:
        {
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("({} + {})", getAsString(left), getAsString(right)));
            break;
        }
        case invokestatic:
        {
            auto id = r16();
            auto method = std::get<Methodref>(constantPool[id]);
            auto className = getStringFromUtf8(std::get<Class>(constantPool[method.class_index]).name_index);
            auto methodName = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).name_index);

            if (className == "java/lang/Integer")
            {
                if (methodName == "valueOf")
                {
                    // do nothing, leave the int on the stack
                    break;
                }
                else
                {
                    throw fmt::format("Method '{}' on class '{}' not handled.", methodName, className);
                }
            }


            auto descriptor = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).descriptor_index);
            auto fullName = methodName;
            if (className != project_name)
            {
                fullName = fmt::format("{}::{}", className, fullName);
            }
            fullName = javaToCpp(fullName);

            std::string argsString;
            auto argsCount = countArgs(descriptor);
            if (argsCount && argsCount <= stack.size())
            {
                auto offset = stack.size() - argsCount;
                for (size_t idx = 0; idx < argsCount; ++idx)
                {
                    argsString += fmt::format(", {}", getAsString(stack[offset + idx]));
                }

                argsString = argsString.substr(2);

                auto tmpArgsCount = argsCount;
                while (tmpArgsCount > 0)
                {
                    --tmpArgsCount;
                    stack.pop_back();
                }
            }

            auto callString = fmt::format("{}({})", fullName, argsString);
            auto retType = getReturnType(descriptor);
            if (retType.size() && retType != "void")
            {
                stack.push_back(callString);
            }
            else
            {
                Operation op;
                op.type = OpType::Call;
                op.call.code = callString + ';';
                operations.push_back(op);
            }
            break;
        }
        case getstatic:
        {
            auto id = r16();
            auto method = std::get<Fieldref>(constantPool[id]);
            auto className = getStringFromUtf8(std::get<Class>(constantPool[method.class_index]).name_index);
            auto descriptor = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).descriptor_index);
            auto variableName = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).name_index);

            auto fullName = variableName;
            if (className != project_name)
            {
                fullName = fmt::format("{}::{}", className, fullName);
            }
            fullName = javaToCpp(fullName);

            stack.push_back(fullName);
            break;
        }
        case sipush:
        {
            auto value = r16();
            stack.push_back(value);
            break;
        }
        case irem:
        {
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("({} % {})", getAsString(left), getAsString(right)));
            break;
        }
        case putstatic:
        {
            auto id = r16();
            auto method = std::get<Fieldref>(constantPool[id]);
            auto className = getStringFromUtf8(std::get<Class>(constantPool[method.class_index]).name_index);
            auto variableName = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).name_index);
            auto descriptor = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).descriptor_index);

            auto fullName = fmt::format("{}::{}", className, variableName);
            fullName = javaToCpp(fullName);

            auto val = stack.back();
            stack.pop_back();

            if (name == STATIC_INIT)
            {
                for (auto & f : fields)
                {
                    if (f.name == variableName)
                    {
                        if (std::holds_alternative<Object>(val))
                        {
                            f.init = std::get<Object>(val).ctor;
                        }
                        else
                        {
                            f.init = getAsString(val);
                        }
                    }
                }
            }
            else
            {
                Operation op;
                op.type = OpType::Call;
                op.call.code = fmt::format("{} = {};", fullName, getAsString(val));
                operations.push_back(op);
            }
            break;
        }
        case newarray:
        {
            auto val = stack.back();
            stack.pop_back();
            auto size = std::get<int>(val);
            auto type = r8();

            Array arr;
            arr.size = size;
            arr.type = getType(type);
            arr.position = start_pc + buffer_size - buffer.size() - 1;
            stack.push_back(arr);
            break;
        }
        case ldc:
        case ldc_w:
        case ldc2_w:
        {
            int index = 0;

            if (opcode == ldc)
            {
                index = r8();
            }
            else
            {
                index = r16();
            }

            auto constant = constantPool[index];

            if (std::holds_alternative<String>(constant))
            {
                auto data = std::get<String>(constant);
                auto str = getStringFromUtf8(data.string_index);
                stack.push_back(fmt::format("\"{}\"", str));
            }
            else if (std::holds_alternative<float>(constant))
            {
                auto f = std::get<float>(constant);
                stack.push_back(fmt::format("{}", f));
            }
            else if (std::holds_alternative<double>(constant))
            {
                auto d = std::get<double>(constant);
                stack.push_back(fmt::format("{}", d));
            }
            else if (std::holds_alternative<s4>(constant))
            {
                auto s = std::get<s4>(constant);
                stack.push_back(fmt::format("{}", s));
            }
            else if (std::holds_alternative<s8>(constant))
            {
                auto s = std::get<s8>(constant);
                stack.push_back(fmt::format("{}L", s));
            }
            else
            {
                throw fmt::format("Trying to load unhandled type. Accepted types: 'int', 'long', 'float', 'double' or 'string'.");
            }
            break;
        }
        case dup_:
        {
            auto val = stack.back();
            stack.push_back(val);
            break;
        }
        case laload:
        case faload:
        case daload:
        case aaload:
        case baload:
        case caload:
        case saload:
        {
            auto index = stack.back();
            stack.pop_back();
            auto arr = stack.back();
            stack.pop_back();
            stack.push_back(fmt::format("{}[{}]", getAsString(arr), getAsString(index)));
            break;
        }
        case l2f:
        {
            auto value = stack.back();
            stack.pop_back();
            stack.push_back(fmt::format("static_cast<float>({})", getAsString(value)));
            break;
        }
        case f2d:
        {
            auto value = stack.back();
            stack.pop_back();
            stack.push_back(fmt::format("static_cast<double>({})", getAsString(value)));
            break;
        }
        case invokespecial:
        {
            auto id = r16();
            auto method = std::get<Methodref>(constantPool[id]);
            auto className = getStringFromUtf8(std::get<Class>(constantPool[method.class_index]).name_index);
            auto methodName = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).name_index);
            auto descriptor = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).descriptor_index);

            auto argsCount = countArgs(descriptor);

            auto objOffset = stack.size() - argsCount - 1;
            auto objRef = getAsString(stack[objOffset]);

            if (!hasBoard() && objRef == OBJ_INSTANCE)
            {
                // remove "this"
                stack.pop_back();

                break;
            }

            std::string fullName;
            if (methodName == CONSTRUCTOR)
            {
                fullName = fmt::format("{}", objRef);
                fullName = javaToCpp(fullName);
            }
            else
            {
                throw fmt::format("invoke special on something that is not a constructor.");
            }

            std::string argsString;

            if (argsCount && argsCount <= stack.size())
            {
                auto offset = stack.size() - argsCount;
                if (className == "gamebuino/Image")
                {
                    if (argsCount > 1 && argsCount < 6)
                    {
                        auto filename = std::get<std::string>(stack[offset + 0]);
                        boost::replace_all(filename, "\""s, ""s);
                        auto sFormat = getAsString(stack[offset + 1]);
                        auto format = (sFormat.ends_with("Rgb565")) ? Format::Rgb565 : Format::Indexed;

                        int yframes = 1, xframes = 1, loop = 0;

                        switch (argsCount)
                        {
                        case 5:
                            loop = std::get<int>(stack[offset + 4]);
                            [[fallthrough]];
                        case 4:
                            xframes = std::get<int>(stack[offset + 3]);
                            [[fallthrough]];
                        case 3:
                            yframes = std::get<int>(stack[offset + 2]);
                            break;
                        }

                        add_resource(filename, format, yframes, xframes, loop);
                        argsString = ", " + encode_filename(filename);
                    }
                    else if (descriptor == "([B)V")
                    {
                        auto array = std::get<std::string>(stack[offset]);
                        argsString = ", " + array;
                    }
                    else if (descriptor == "([S)V")
                    {
                        auto array = std::get<std::string>(stack[offset]);
                        argsString = ", " + array;
                    }
                    else
                    {
                        assert(false);
                    }
                }
                else
                {
                    for (size_t idx = 0; idx < argsCount; ++idx)
                    {
                        argsString += fmt::format(", {}", getAsString(stack[offset + idx]));
                    }
                }

                argsString = argsString.substr(2);

                auto tmpArgsCount = argsCount;
                while (tmpArgsCount > 0)
                {
                    --tmpArgsCount;
                    stack.pop_back();
                }
            }

            // remove "this"
            stack.pop_back();

            auto callString = fmt::format("{}({})", fullName, argsString);
            if (objRef == className)
            {
                stack.pop_back();

                Object obj;
                obj.type = javaToCpp(className);
                obj.ctor = callString;
                stack.push_back(obj);
            }
            else
            {
                callString += ';';
                auto retType = getReturnType(descriptor);
                if ((retType.size() && retType != "void"))
                {
                    stack.push_back(callString);
                }
                else
                {
                    Operation op;
                    op.type = OpType::Call;
                    op.call.code = callString;
                    operations.push_back(op);
                }
            }
            break;
        }
        case invokevirtual:
        {
            auto id = r16();
            auto method = std::get<Methodref>(constantPool[id]);
            auto className = getStringFromUtf8(std::get<Class>(constantPool[method.class_index]).name_index);
            auto methodName = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).name_index);
            auto descriptor = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).descriptor_index);

            auto argsCount = countArgs(descriptor);

            auto objOffset = stack.size() - argsCount - 1;
            auto objRef = getAsString(stack[objOffset]);

            auto fullName = methodName;
            std::string ths = getAsString(objRef);
            if (hasBoard() || ths != OBJ_INSTANCE)
            {
                fullName = fmt::format("{}.{}", objRef, fullName);
            }
            fullName = javaToCpp(fullName);

            std::string argsString;
            if (argsCount && argsCount <= stack.size())
            {
                auto offset = stack.size() - argsCount;
                if (fullName == "gamebuino::gb::display.printf")
                {
                    argsString += fmt::format(", {}", getAsString(stack[offset]));
                    auto arr = std::get<Array>(stack[offset + 1]);
                    for (auto p : arr.populate)
                    {
                        argsString += fmt::format(", {}", p);
                    }
                }
                else
                {
                    for (size_t idx = 0; idx < argsCount; ++idx)
                    {
                        argsString += fmt::format(", {}", getAsString(stack[offset + idx]));
                    }
                }

                argsString = argsString.substr(2);

                auto tmpArgsCount = argsCount;
                while (tmpArgsCount > 0)
                {
                    --tmpArgsCount;
                    stack.pop_back();
                }
            }

            // remove "this"
            stack.pop_back();

            auto callString = fmt::format("{}({})", fullName, argsString);
            auto retType = getReturnType(descriptor);
            if (retType.size() && retType != "void")
            {
                stack.push_back(callString);
            }
            else
            {
                Operation op;
                op.type = OpType::Call;
                op.call.code = callString + ';';
                operations.push_back(op);
            }
            break;
        }
        case ifeq:
        case ifne:
        case iflt:
        case ifge:
        case ifgt:
        case ifle:
        {
            auto correction = buffer_size - buffer.size() - 1;
            auto offset = s16();
            auto absolute = start_pc + correction + offset;

            auto value = stack.back();
            stack.pop_back();

            std::string binop;
            switch (opcode)
            {
            case ifeq: binop = "!="; break;
            case ifne: binop = "=="; break;
            case iflt: binop = ">="; break;
            case ifge: binop =  "<"; break;
            case ifgt: binop = "<="; break;
            case ifle: binop =  ">"; break;
            default: assert(false);
            }


            Operation op;
            op.type = OpType::Cond;
            op.cond.op = binop;
            op.cond.left = getAsString(value);
            op.cond.right = "0";
            op.cond.absolute = absolute;

            operations.push_back(op);

            if ((*fullBuffer)[absolute - 3] == goto_)
            {
                skippedGotos.insert(absolute - 3);
            }
            break;
        }
        case new_:
        {
            auto index = r16();
            Object obj;
            obj.type = getStringFromUtf8(std::get<Class>(constantPool[index]).name_index);
            stack.push_back(obj);
            break;
        }
        case idiv:
        {
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("({} / {})", getAsString(left), getAsString(right)));
            break;
        }
        case isub:
        {
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("({} - {})", getAsString(left), getAsString(right)));
            break;
        }
        case getfield:
        {
            auto index = r16();
            auto objRef = stack.back();
            stack.pop_back();

            auto field = std::get<Fieldref>(constantPool[index]);
            auto fieldName = getStringFromUtf8(std::get<NameAndType>(constantPool[field.name_and_type_index]).name_index);

            std::string ths = getAsString(objRef);
            if (!hasBoard() && ths == OBJ_INSTANCE)
            {
                stack.push_back(fmt::format("{}", fieldName));
            }
            else
            {
                stack.push_back(fmt::format("{}.{}", ths, fieldName));
            }
            break;
        }
        case putfield:
        {
            auto index = r16();
            auto value = stack.back();
            stack.pop_back();
            auto objRef = stack.back();
            stack.pop_back();

            auto field = std::get<Fieldref>(constantPool[index]);
            auto fieldName = getStringFromUtf8(std::get<NameAndType>(constantPool[field.name_and_type_index]).name_index);

            Operation op;
            op.type = OpType::Call;

            std::string ths = getAsString(objRef);
            if (!hasBoard() && ths == OBJ_INSTANCE)
            {
                op.call.code = fmt::format("{} = {};", fieldName, getAsString(value));
            }
            else
            {
                op.call.code = fmt::format("{}.{} = {};", ths, fieldName, getAsString(value));
            }

            operations.push_back(op);
            break;
        }
        case ineg:
        {
            auto value = stack.back();
            stack.pop_back();
            stack.push_back(fmt::format("-{}", getAsString(value)));
            break;
        }
        case iconst_m1:
        {
            stack.push_back(-1);
            break;
        }
        case iand:
        {
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("({} & {})", getAsString(left), getAsString(right)));
            break;
        }
        case aconst_null:
        {
            stack.push_back("null");
            break;
        }
        default:
            throw fmt::format("Unhandled opcode: '{:x}'.", opcode);
        }
    }

    bool addOpeningParen = false;
    bool parsed = false;
    switch (operations.size())
    {
    case 0: // <(cl)init>
        if (name.front() != '<')
        {
            assert(false);
        }
        parsed = true;
        break;
    case 2:
    {
        const auto & o1 = operations[0];
        const auto & o2 = operations[1];

        if (o2.type == OpType::Jump)
        {
            inst.opcode = generateCodeFromOperation(o1, start_pc, addOpeningParen);

            if (o2.jump.absolute > start_pc)
            {
                Instruction tmp;
                tmp.position = position;
                tmp.opcode = generateCodeFromOperation(o2, start_pc, addOpeningParen); //???
                lineInsts.push_back(tmp);
            }
            else
            {
                auto jumpLine = getLineFromOpcode(o2.jump.absolute);
                bool append = true;
                for (size_t idx = 0; idx < insts.size(); ++idx)
                {
                    if (jumpLine <= insts[idx].position)
                    {
                        Instruction tmp;
                        tmp.position = position;
                        tmp.opcode = "while (true)";
                        insts.insert(insts.begin() + idx, tmp);
                        tmp.opcode = "{";
                        insts.insert(insts.begin() + idx + 1, tmp);
                        append = false;
                        break;
                    }
                }

                if (append)
                {
                    Instruction tmp;
                    tmp.position = position;
                    tmp.opcode = "while (true)";
                    insts.push_back(tmp);
                    tmp.opcode = "{";
                    insts.push_back(tmp);
                }

                Instruction tmp;
                tmp.position = position;
                tmp.opcode = "}";
                lineInsts.push_back(tmp);
            }

            parsed = true;
        }
        else if (o1.type == OpType::Cond && o2.type == OpType::Cond)
        {
            auto & c1 = operations[0].cond;
            auto & c2 = operations[1].cond;
            auto absolute1 = c1.absolute;
            auto absolute2 = c2.absolute;
            //auto isGoto = (*fullBuffer)[absolute1 - 3] == goto_ || (*fullBuffer)[absolute2 - 3] == goto_;

            std::string andOrOr;
            if (absolute1 == absolute2)
            {
                // AND
                andOrOr = "&&";
            }
            else
            {
                // OR
                andOrOr = "||";
                c1.op = invertBinaryOperator(c1.op);
            }

            inst.opcode = fmt::format("if ({} {} {} {} {} {} {})",
                                      c1.left, c1.op, c1.right,
                                      andOrOr,
                                      c2.left, c2.op, c2.right);
            addOpeningParen = true;
            closingBrackets.insert(getLineFromOpcode(absolute2));

            parsed = true;
        }
        break;
    }
    case 4:
    {
        const auto & o1 = operations[0];
        const auto & o2 = operations[1];
        const auto & o3 = operations[2];
        const auto & o4 = operations[3];

        // for loop
        if (o1.type == OpType::Store && o2.type == OpType::Cond && o3.type == OpType::Inc && o4.type == OpType::Jump)
        {
            std::string loop_var_type;
            if (o1.store.type.has_value())
            {
                loop_var_type = o1.store.type.value() + " ";
            }
            inst.opcode = fmt::format("for ({0}local_{1} = {2}; {3} {4} {5}; local_{6}",
                                      loop_var_type, o1.store.index, o1.store.value.value(),
                                      o2.cond.left, o2.cond.op, o2.cond.right,
                                      o3.inc.index);
            if (o3.inc.constant == 1)
            {
                inst.opcode += "++";
            }
            else
            {
                inst.opcode += fmt::format(" += {}", o3.inc.constant);
            }
            inst.opcode += ")";

            closingBrackets.insert(getLineFromOpcode(o2.cond.absolute));
            addOpeningParen = true;

            parsed = true;
        }
        break;
    }
    }

    if (!parsed)
    {
        auto op = operations[0];
        inst.opcode = generateCodeFromOperation(op, start_pc, addOpeningParen);

        for (size_t size = 1; size < operations.size(); ++size)
        {
            if (operations[size-1].type == OpType::Cond)
            {
                Instruction openingBracket;
                openingBracket.position = position;
                openingBracket.opcode = "{";
                lineInsts.push_back(openingBracket);

                localsTypes.push_back({});
                addOpeningParen = false;
            }

            Instruction tmp;
            tmp.position = position;
            tmp.opcode = generateCodeFromOperation(operations[size], start_pc, addOpeningParen);
            lineInsts.push_back(tmp);
        }
    }

    for (auto it = begin(elseStmts); it != end(elseStmts);)
    {
        if (*it <= position)
        {
            Instruction elseStmt;
            elseStmt.position = position;
            elseStmt.opcode = "{";
            lineInsts.insert(begin(lineInsts), elseStmt);
            elseStmt.opcode = "else";
            lineInsts.insert(begin(lineInsts), elseStmt);

            it = elseStmts.erase(it);

            localsTypes.pop_back();     // remove previous scope
            localsTypes.push_back({});  // add clean scope
            localsTypes.push_back({});  // add dummy scope that will get removed next loop
        }
        else
        {
            ++it;
        }
    }

    for (auto it = begin(closingBrackets); it != end(closingBrackets);)
    {
        if (*it <= position)
        {
            Instruction closingBracket;
            closingBracket.position = position;
            closingBracket.opcode = "}";
            lineInsts.insert(begin(lineInsts), closingBracket);
            it = closingBrackets.erase(it);

            localsTypes.pop_back();
        }
        else
        {
            ++it;
        }
    }

    if (addOpeningParen)
    {
        Instruction openingBracket;
        openingBracket.position = position;
        openingBracket.opcode = "{";
        lineInsts.push_back(openingBracket);

        localsTypes.push_back({});
    }

    return lineInsts;
}

std::string ClassFile::generateCodeFromOperation(Operation operation, u4 start_pc, bool & addOpeningParen)
{
    std::string output;

    switch (operation.type)
    {
    case OpType::Store:
    {
        auto s = operation.store;
        std::string tmp;

        // if not an array print the optional type
        if (s.type.has_value() && !s.size.has_value())
        {
            tmp += s.type.value();
            tmp += " ";
        }

        // array
        if (s.size.has_value())
        {
            tmp += fmt::format("{} temp_{:x}[{}]", s.arr_type, s.position, s.size.value());
        }
        else
        {
            // variable
            tmp += fmt::format("local_{}", s.index);
            if (s.value.has_value())
            {
                tmp += fmt::format(" = {}", s.value.value());
            }
        }

        // initialise local from array
        if (s.size.has_value())
        {
            for (size_t p = 0; p < s.populate.size(); ++p)
            {
                tmp += fmt::format("; temp_{:x}[{}] = {}", s.position, p, s.populate[p]);
            }

            std::string type = "";
            if (s.type.has_value())
            {
                type = s.type.value() + "* ";
            }
            tmp += fmt::format("; {}local_{} = temp_{:x}", type, s.index, s.position);
        }

        output = tmp + ";";
        break;
    }
    case OpType::IndexedStore:
    {
        output = fmt::format("{}[{}] = {};", operation.istore.array, operation.istore.index, operation.istore.value);
        break;
    }
    case OpType::Return:
    {
        auto & val = operation.ret.value;
        if (val.has_value())
        {
            output = fmt::format("return {};", val.value());
        }
        else
        {
            output = fmt::format("return;");
        }
        break;
    }
    case OpType::Cond:
    {
        auto & c = operation.cond;
        auto absolute = c.absolute;
        auto isGoto = (*fullBuffer)[absolute - 3] == goto_;
        addOpeningParen = true;

        auto l = c.left;
        auto binop = c.op;
        auto r = c.right;

        if (isGoto)
        {
            s2 offset = (*fullBuffer)[absolute - 2] << 8 | (*fullBuffer)[absolute - 1];
            u4 target = absolute - 3 + offset;

            if (target == start_pc)
            {
                output = fmt::format("while ({} {} {})", l, binop, r);
                closingBrackets.insert(getLineFromOpcode(absolute));
            }
            else if (target > start_pc)
            {
                output = fmt::format("if ({} {} {})", l, binop, r);
                closingBrackets.insert(getLineFromOpcode(absolute));
            }
            else
            {
                throw fmt::format("A condition operation wants to jump backwards.");
            }
        }
        else
        {
            output = fmt::format("if ({} {} {})", l, binop, r);
            closingBrackets.insert(getLineFromOpcode(absolute));
        }
        break;
    }
    case OpType::Inc:
    {
        if (operation.inc.constant == 1)
        {
            output = fmt::format("local_{}++;", operation.inc.index);
        }
        else
        {
            output = fmt::format("local_{} += {};", operation.inc.index, operation.inc.constant);
        }
        break;
    }
    case OpType::Jump:
    {
        auto abs_line = getLineFromOpcode(operation.jump.absolute);
        auto curr_line = getLineFromOpcode(start_pc);
        if (operation.jump.absolute > start_pc)
        {
            if (abs_line < curr_line)
            {
                for (u4 pos = operation.jump.absolute; pos < fullBuffer->size(); ++pos)
                {
                    auto next_line = getLineFromOpcode(pos);
                    if (next_line > curr_line)
                    {
                        abs_line = next_line;
                        break;
                    }
                }
            }

            if (abs_line <= curr_line)
            {
                throw fmt::format("A jump operation didn't found its target.");
            }

            elseStmts.insert(curr_line + 1);
            closingBrackets.insert(abs_line);
        }
        else
        {
            throw fmt::format("jump operations backwards are not handled.");
        }
        break;
    }
    case OpType::Call:
    {
        output = operation.call.code;
        break;
    }
    default:
        throw fmt::format("Invalid operation: '{}'.", std::to_underlying(operation.type));
    }

    return output;
}

u2 ClassFile::getLineFromOpcode(u2 opcode)
{
    u2 lastLineSaw = 0;

    for (size_t i = 0; i < lines->size() - 1; ++i)
    {
        auto pc = std::get<0>((*lines)[i]);
        auto nb = std::get<1>((*lines)[i]);
        if (lastLineSaw <= nb)
        {
            if (opcode <= pc)
            {
                return nb;
            }
            lastLineSaw = nb;
        }
    }

    return std::get<1>((*lines)[lines->size() - 1]);
}

u2 ClassFile::getOpcodeFromLine(u2 line)
{
    for (auto & l : *lines)
    {
        if (std::get<1>(l) == line)
        {
            return std::get<0>(l);
        }
    }

    throw fmt::format("Invalid line given.");
}

int ClassFile::findLocal(int index)
{
    for (auto it = localsTypes.rbegin(); it != localsTypes.rend(); ++it)
    {
        if (it->contains(index))
        {
            return it->operator [](index);
        }
    }

    return T_NONE;

}

std::string ClassFile::getStringFromUtf8(int index)
{
    auto b = std::get<Utf8>(constantPool[index]).bytes;
    return std::string { begin(b), end(b) };
}

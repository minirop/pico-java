#include "lineanalyser.h"
#include <cassert>

using namespace std::string_literals;

enum
{
    T_NONE    = 0,
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

    assert(false);
}

std::vector<Instruction> decodeBytecodeLine(Buffer & buffer, const std::string & name, u4 position);

std::vector<std::unordered_map<u4, u4>> localsTypes;
std::vector<Instruction> insts;
std::unordered_map<u4, u4> closingBraces;
std::vector<Instruction> lineAnalyser(Buffer & buffer, const std::string & name, std::vector<u2> lineNumbers)
{
    insts.clear();
    localsTypes.push_back({});
    closingBraces.clear();

    for (size_t i = 0; i < lineNumbers.size(); ++i)
    {
        auto begin = lineNumbers[i];
        auto end = (i + 1 < lineNumbers.size() ? lineNumbers[i + 1] : buffer.size());
        auto sub = Buffer(&buffer[begin], &buffer[end]);

        auto ret = decodeBytecodeLine(sub, name, begin);
        insts.insert(
                    insts.end(),
                    std::make_move_iterator(ret.begin()),
                    std::make_move_iterator(ret.end())
                    );
    }

    return insts;
}

int findLocal(int index)
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

bool previousWasElse = false;
std::vector<Instruction> decodeBytecodeLine(Buffer & buffer, const std::string & name, u4 position)
{
    std::vector<Instruction> lines;
    std::vector<std::string> stack;

    auto buffer_size = buffer.size();

    if (closingBraces.contains(position))
    {
        auto count = closingBraces[position];
        while (count > 0)
        {

            lines.push_back(Instruction {
                                UNDEFINED_POSITION,
                                "}",
                                {}
                            });
            if (localsTypes.size())
                localsTypes.pop_back();
            --count;
        }
    }

    while (buffer.size())
    {
        auto opcode = r8();

        switch (opcode)
        {
        case iconst_0:
        case iconst_1:
        case iconst_2:
        case iconst_3:
        case iconst_4:
        case iconst_5:
        {
            stack.push_back(fmt::format("{}", opcode - iconst_0));
            break;
        }
        case getstatic:
        {
            auto id = r16();
            auto method = std::get<Fieldref>(constantPool[id]);
            auto className = getStringFromUtf8(std::get<Class>(constantPool[method.class_index]).name_index);
            auto descriptor = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).descriptor_index);
            auto variableName = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).name_index);

            auto fullName = fmt::format("{}::{}", className, variableName);
            boost::replace_all(fullName, "/"s, "::"s);

            stack.push_back(fullName);
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
            boost::replace_all(fullName, "/"s, "::"s);

            if (descriptor == "I")
            {
                auto val = stack.back();
                lines.push_back(Instruction {
                                    position,
                                    fmt::format("{} = {};", fullName, val),
                                    {}
                                });
                stack.pop_back();
            }
            else
            {
                assert(false);
            }
            break;
        }
        case invokestatic:
        {
            auto id = r16();
            auto method = std::get<Methodref>(constantPool[id]);
            auto className = getStringFromUtf8(std::get<Class>(constantPool[method.class_index]).name_index);
            auto methodName = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).name_index);
            auto descriptor = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).descriptor_index);
            auto fullName = fmt::format("{}::{}", className, methodName);
            boost::replace_all(fullName, "/"s, "::"s);

            std::string argsString;

            if (methodName == "set_irq_enabled_with_callback")
            {
                std::string ss;
            }

            auto argsCount = countArgs(descriptor);
            if (argsCount && argsCount <= stack.size())
            {
                auto offset = stack.size() - argsCount;
                for (size_t idx = 0; idx < argsCount; ++idx)
                {
                    argsString += fmt::format(", {}", stack[offset + idx]);
                }

                argsString = argsString.substr(2);

                auto tmpArgsCount = argsCount;
                while (tmpArgsCount > 0)
                {
                    --tmpArgsCount;
                    stack.pop_back();
                }
            }

            auto callString = fmt::format("{}({});", fullName, argsString);
            auto retType = getReturnType(descriptor);
            if (retType.size() && retType != "void")
            {
                auto varName = fmt::format("var_0x{:x}", position);
                callString = fmt::format("auto {} = {}", varName, callString);
                stack.push_back(varName);
            }

            lines.push_back(Instruction {
                                position,
                                callString,
                                {}
                            });
            break;
        }
        case bipush:
        {
            auto value = r8();
            stack.push_back(fmt::format("{}", value));
            break;
        }
        case sipush:
        {
            auto value = r16();
            stack.push_back(fmt::format("{}", value));
            break;
        }
        case iload:
        {
            auto index = r8();
            stack.push_back(fmt::format("ilocal_{}", index));
            break;
        }
        case iload_0:
        case iload_1:
        case iload_2:
        case iload_3:
        {
            int index = opcode - iload_0;
            stack.push_back(fmt::format("ilocal_{}", index));
            break;
        }
        case aload_0:
        case aload_1:
        case aload_2:
        case aload_3:
        {
            int index = opcode - aload_0;
            stack.push_back(fmt::format("alocal_{}", index));
            break;
        }
        case iaload:
        {
            auto index = stack.back();
            stack.pop_back();
            auto array = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("{}[{}]", array, index));
            break;
        }
        case istore:
        {
            auto v = stack.back();
            stack.pop_back();

            auto index = r8();
            std::string type;

            auto & locals = localsTypes.back();
            auto localType = findLocal(index);
            if (localType != T_INT)
            {
                type = getType(T_INT);
                locals[index] = T_INT;
            }
            lines.push_back(Instruction {
                                UNDEFINED_POSITION,
                                fmt::format("{} ilocal_{} = {};", type, index, v),
                                {}
                            });
            break;
        }
        case istore_0:
        case istore_1:
        case istore_2:
        case istore_3:
        {
            auto v = stack.back();
            stack.pop_back();

            int index = opcode - istore_0;
            std::string type;

            auto & locals = localsTypes.back();
            auto localType = findLocal(index);
            if (localType != T_INT)
            {
                type = getType(T_INT);
                locals[index] = T_INT;
            }
            lines.push_back(Instruction {
                                UNDEFINED_POSITION,
                                fmt::format("{} ilocal_{} = {};", type, index, v),
                                {}
                            });
            break;
        }
        case astore_0:
        case astore_1:
        case astore_2:
        case astore_3:
        {
            int index = opcode - astore_0;
            auto v = stack.back();
            stack.pop_back();
            auto & locals = localsTypes.back();
            auto localType = findLocal(index);
            if (localType != T_ARRAY)
            {
                lines.push_back(Instruction {
                                    UNDEFINED_POSITION,
                                    fmt::format("auto alocal_{} = {};", index, v),
                                    {}
                                });
                locals[index] = T_ARRAY;
            }
            break;
        }
        case iastore:
        {
            auto value = stack.back();
            stack.pop_back();
            auto index = stack.back();
            stack.pop_back();
            auto arr = stack.back();
            stack.pop_back();

            lines.push_back(Instruction {
                                position,
                                fmt::format("{}[{}] = {};", arr, index, value),
                                {}
                            });
            break;
        }
        case pop:
        {
            stack.pop_back();
            break;
        }
        case dup_:
        {
            auto val = stack.back();
            stack.push_back(val);
            break;
        }
        case iadd:
        {
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("({} + {})", left, right));
            break;
        }
        case irem:
        {
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("({} % {})", left, right));
            break;
        }
        case ishl:
        {
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("({} << {})", left, right));
            break;
        }
        case iinc:
        {
            auto value = r8();
            auto inc = r8();

            lines.push_back(Instruction {
                                position,
                                fmt::format("ilocal_{} += {};", value, inc),
                                {}
                            });
            break;
        }
        case ifne:
        {
            previousWasElse = false;

            auto correction = buffer_size - buffer.size() - 1;
            auto offset = s16();
            auto absolute = position + correction + offset;

            auto value = stack.back();
            stack.pop_back();

            lines.push_back(Instruction {
                                position,
                                fmt::format("if ({} == 0) {{", value, name, absolute),
                                {}
                            });

            if (!closingBraces.contains(absolute))
            {
                closingBraces[absolute] = 0;
            }
            closingBraces[absolute]++;

            localsTypes.push_back({});
            break;
        }
        case if_icmpeq:
        case if_icmpne:
        case if_icmplt:
        case if_icmpge:
        case if_icmpgt:
        case if_icmple:
        {
            previousWasElse = false;

            auto correction = buffer_size - buffer.size() - 1;
            auto offset = s16();
            auto absolute = position + correction + offset;

            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            std::string op;
            switch (opcode)
            {
            case if_icmpeq: op = "!="; break;
            case if_icmpne: op = "=="; break;
            case if_icmplt: op = ">="; break;
            case if_icmpge: op =  "<"; break;
            case if_icmpgt: op = "<="; break;
            case if_icmple: op =  ">"; break;
            default: assert(false);
            }

            lines.push_back(Instruction {
                                position,
                                fmt::format("if ({} {} {}) {{", left, op, right, name, absolute),
                                {}
                            });

            if (!closingBraces.contains(absolute))
            {
                closingBraces[absolute] = 0;
            }
            closingBraces[absolute]++;

            localsTypes.push_back({});
            break;
        }
        case goto_:
        {
            auto correction = buffer_size - buffer.size() - 1;
            auto offset = s16();
            auto absolute = position + correction + offset;
            if (offset < 0)
            {
                bool found = false;
                for (u4 iii = 0; iii < insts.size() && !found; ++iii)
                {
                    if (insts[iii].position == absolute)
                    {
                        insts.insert(insts.begin() + iii, Instruction {
                                         position,
                                         "while (true) {",
                                         {}
                                     });
                        found = true;
                    }
                }

                if (!found)
                {
                    for (u4 iii = 0; iii < lines.size() && !found; ++iii)
                    {
                        if (lines[iii].position == absolute)
                        {
                            lines.insert(lines.begin() + iii, Instruction {
                                             position,
                                             "while (true) {",
                                             {}
                                         });
                            found = true;
                        }
                    }
                }
                assert(found);
                lines.push_back(Instruction {
                                    position,
                                    "}",
                                    {}
                                });
            }
            else
            {
                if (previousWasElse)
                {
                    lines.push_back(Instruction {
                                        position,
                                        "}",
                                        {}
                                    });
                    closingBraces[absolute]--;
                    localsTypes.pop_back();
                }
                lines.push_back(Instruction {
                                    position,
                                    fmt::format("}} else {{"),
                                    {}
                                });
                previousWasElse = true;
                if (!closingBraces.contains(absolute))
                {
                    closingBraces[absolute] = 0;
                }
                closingBraces[absolute]++;

                if (closingBraces.contains(position + correction + 3))
                {
                    closingBraces[position + correction + 3] = 0;
                }

                if (localsTypes.size())
                    localsTypes.pop_back();
                localsTypes.push_back({});
            }
            break;
        }
        case iand:
        {
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("({} & {})", left, right));
            break;
        }
        case return_:
        {
            assert (stack.size() == 0);

            lines.push_back(Instruction {
                                position,
                                "return;",
                                {}
                            });
            break;
        }
        case invokedynamic:
        {
            auto id = r16();
            auto zero = r16();
            assert(zero == 0);

            auto invokeDyn = std::get<InvokeDynamic>(constantPool[id]);

            stack.push_back(callbacksMethods[invokeDyn.bootstrap_method_attr_index]);
            break;
        }
        case newarray:
        {
            auto val = stack.back();
            stack.pop_back();
            auto size = std::stoi(val);
            auto type = r8();
            //assert(type == T_INT);

            lines.push_back(Instruction {
                                UNDEFINED_POSITION,
                                fmt::format("{} array_0x{:x}[{}];", getType(type), position, size),
                                {}
                            });
            stack.push_back(fmt::format("array_0x{:x}", position));
            break;
        }
        case arraylength:
        {
            auto val = stack.back();
            stack.pop_back();
            stack.push_back(fmt::format("{}.length", val));
            break;
        }
        default:
            assert(false);
        }
    }

    assert(!stack.size());

    return lines;
}
